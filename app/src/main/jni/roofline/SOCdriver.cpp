#include "SOCdriver.hpp"
#include "GPUdriver.hpp"
#include "Utils.hpp"
#include "CPUkernel.hpp"
#include "Barrier.hpp"

#include <map>

#include <string>
#include <unistd.h>
#include <thread>

struct arg_struct {
    struct input_struct {
        float *buff = NULL;
        int size = 0;
        int flops = 0;

        struct cpu_struct {
            int threads;
        } cpu;

        struct gpu_struct {
            int workgroup_size;
            int workgroup_items;
        } gpu;
    } input;

    struct output {
        double time;
    } output;
};

static JNIEnv *JavaEnv;
static jobject JavaObj;

JNIEXPORT jstring JNICALL Java_com_google_gables_Roofline_SOCTest(JNIEnv *env,
                                                                  jobject thiz) {
    std::string str = "Hello from CPU Roofline!";
    return env->NewStringUTF(str.c_str());
}

void runCPU(struct arg_struct *args) {

    double startTime, endTime;
    int bytes_per_elem, flops_per_elem, mem_accesses_per_elem, n_flops;

    omp_set_dynamic(0);
    omp_set_num_threads(args->input.cpu.threads);

    n_flops = args->input.flops;
    CPUKernelPtr kptr = CPU_get_kernel(n_flops);

    // determine thread chunk size
    uint64_t nsize = (args->input.size / args->input.cpu.threads);
    nsize = nsize & (~(ALIGN - 1));
    nsize = nsize / sizeof(float);

    // now wait for all other agents to be ready
    Barrier_Wait();

#pragma omp parallel
    {
        int id = omp_get_thread_num();
        uint64_t nid = nsize * id;

        LOGD("CPU ThreadID (%d) array nsize = %llu bytes (%llu MB)", id, nsize, nsize / MiB);

        // get the CPU threads to barrier sync
#pragma omp barrier
#pragma omp master
        {
            startTime = getOMPTime();
        }

        kptr(nsize, 1, &args->input.buff[nid], &bytes_per_elem, &flops_per_elem,
             &mem_accesses_per_elem);

#pragma omp barrier
#pragma omp master
        {
            endTime = getOMPTime();
        }
    }

    double total_seconds = (endTime - startTime);
    args->output.time = total_seconds;
}

void runGPU(struct arg_struct *args) {

    GPUInitializeOpenGL();

#ifdef DEBUG
    // ideally we can reuse this function but currently this is setup to sweep,
    // which makes it hard to reuse -- but in the future we can try to reuse code.

    Java_com_google_gables_Roofline_GPUExecute(JavaEnv, JavaObj, (jobject) NULL,
                                               args->input.gpu.workgroup_size,
                                               args->input.gpu.workgroup_items,
                                               args->input.flops,
                                               args->input.size);
#endif

    GPUKernelPtr kernel = GPUBuildKernel(args->input.gpu.workgroup_items, args->input.flops);

    int n_size = args->input.size;
    GPUConfigureSSBO(n_size, 1);

    Barrier_Wait();

    double startTime = getTime();
    GPULaunchKernel(kernel, args->input.gpu.workgroup_size);
    double endTime = getTime();

    double total_seconds = endTime - startTime;
    args->output.time = total_seconds;

    // fixme: there is a memory leak here. need to clean up the buffers
    // this will not be an issue if we can use the above code.

    GPUShutdownOpenGL();
}

// invoke the processing engines in parallel, using pthreads
double run(int mem_size, int flops, float cpu_frac, int cpu_threads, int gpu_workgroup_size,
           int gpu_workitem_size) {

    // first determine how much memory each agent is processing
    int cpu_size = (int) (mem_size * cpu_frac);
    int gpu_size = mem_size - cpu_size;
    LOGD("Assigning CPU %d bytes (%d MB)", cpu_size, cpu_size / MiB);
    LOGD("Assigning GPU %d bytes (%d MB)", gpu_size, gpu_size / MiB);

    struct arg_struct cpu_args;
    struct arg_struct gpu_args;

    memset(&cpu_args, 0, sizeof(arg_struct));
    memset(&gpu_args, 0, sizeof(arg_struct));

    // configure cpu params
    cpu_args.input.buff = (float *) memalign(16, (size_t) cpu_size);
    cpu_args.input.size = cpu_size;
    cpu_args.input.flops = flops;
    cpu_args.input.cpu.threads = cpu_threads;
    assert(cpu_args.input.buff);

    // configure gpu params
    gpu_args.input.buff = NULL; // allocated via SSBO
    gpu_args.input.size = gpu_size;
    gpu_args.input.flops = flops;
    gpu_args.input.gpu.workgroup_size = gpu_workgroup_size;
    gpu_args.input.gpu.workgroup_items = gpu_workitem_size;

    double executionTime;

    if (cpu_frac == 1.0) {
        // the work is assigned only to CPU
        Barrier_Init(1);

        runCPU(&cpu_args);

        executionTime = cpu_args.output.time;
    } else if (cpu_frac == 0.0) {
        // the work is assigned ONLY to GPU
        Barrier_Init(1);

        runGPU(&gpu_args);

        executionTime = gpu_args.output.time;
    } else {
        // setup a barrier to synchronize CPU and GPU
        Barrier_Init(2);

        // launch the threads
        std::thread cpu_thread(runCPU, &cpu_args);
        std::thread gpu_thread(runGPU, &gpu_args);

        // wait for threads to finish
        cpu_thread.join();
        gpu_thread.join();

        LOGD("CPU ran for %f seconds.", cpu_args.output.time);
        LOGD("GPU ran for %f seconds.", gpu_args.output.time);

        executionTime = (cpu_args.output.time > gpu_args.output.time ?
                         cpu_args.output.time : gpu_args.output.time);
    }

    // cleanup
    if (cpu_args.input.buff)
        free(cpu_args.input.buff);

    LOGD("Raw execution time = %f seconds", executionTime);
    return executionTime;
}

std::string runSweepExperiment(int memTotal, int flops, float cpu_work_frac, int cpu_threads,
                               int gpu_workgroup_size, int gpu_workitem_size) {

    // data types to do a map lookup
    typedef std::map<float, double> MapFracTime_t;
    typedef std::map<int, MapFracTime_t> MapResults_t;

    MapResults_t results;
    std::ostringstream results_ostr;

#define ITERATIONS (2)
#define MAX_FLOPS (1024)

    // run the experiment N number of times to get the data
    for (int flops = 1; flops <= MAX_FLOPS; flops *= 2) {
        for (float frac = 0.0; frac <= 1.0; frac += 0.125) {
            float cpu_work_frac = 1.0 - frac;
            double sumTime = 0.0;
            for (int i = 0; i < ITERATIONS; i++) {
                double runTime = run(memTotal, flops, cpu_work_frac, cpu_threads,
                                     gpu_workgroup_size, gpu_workitem_size);
                sumTime += runTime;
            }
            results[flops][frac] = sumTime / ITERATIONS / flops;
        }
        LOGI("");
    }

    // "print" the headers
    for (MapResults_t::iterator it = results.begin(); it != results.end(); it++) {
        results_ostr << "FLOPS";
        for (MapFracTime_t::iterator inner_it = (*it).second.begin();
             inner_it != (*it).second.end(); inner_it++) {
            results_ostr << "," << (*inner_it).first; // details
        }
        results_ostr << std::endl;
        break;
    }

    // now dump out the real values
    for (MapResults_t::iterator it = results.begin(); it != results.end(); it++) {
        results_ostr << (*it).first;
        for (MapFracTime_t::iterator inner_it = (*it).second.begin();
             inner_it != (*it).second.end(); inner_it++) {
            results_ostr << "," << (*inner_it).second; // details
        }
        results_ostr << std::endl;
    }

    LOGE("%s", results_ostr.str().c_str());
    return results_ostr.str();
}

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_SOCMixer(JNIEnv *env, jobject thiz, jint memTotal, jint flops,
                                         jfloat cpu_work_frac, jint cpu_threads,
                                         jint gpu_workgroup_size,
                                         jint gpu_workitem_size) {
    // global variables for class
    JavaEnv = env;
    JavaObj = thiz;

    std::string results = runSweepExperiment(memTotal, flops,
                                             cpu_work_frac, cpu_threads,
                                             gpu_workgroup_size,
                                             gpu_workitem_size);

    return env->NewStringUTF(results.c_str());
}