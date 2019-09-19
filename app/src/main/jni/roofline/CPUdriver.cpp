#include <unistd.h>
#include "CPUdriver.hpp"
#include "CPUkernel.hpp"

double getTime() {
    double time;

    time = omp_get_wtime();
    return time;
}

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_CPUExecute(JNIEnv *env, jobject obj,
                                           jint memTotal, jint nThreads, jint nFlops,
                                           jboolean neon) {
    int id = 0;

    assert(nThreads >= 1);

    std::ostringstream results;
    results.str("");
    uint64_t TSIZE = 100000000;

    double *__restrict__ buf = (double *) malloc(TSIZE);

    if (buf == NULL) {
        std::string str = "Error in CPU Roofline!";
        return env->NewStringUTF("hi");
    }

    omp_set_dynamic(0);             // Explicitly disable dynamic teams
    omp_set_num_threads(nThreads);  // Use nThreads for all consecutive parallel regions

    // based on the number of flops specified, pick the appropriate kernel to run
    void (*kernelPtr)(uint64_t, uint64_t, double *__restrict__, int *, int *);
    switch (nFlops) {
        case 1:
            kernelPtr = &kernel_1FLOPS;
            break;
        case 2:
            kernelPtr = &kernel_2FLOPS;
            break;
        case 4:
            kernelPtr = &kernel_4FLOPS;
            break;
        case 8:
            kernelPtr = &kernel_8FLOPS;
            break;
        case 16:
            kernelPtr = &kernel_16FLOPS;
            break;
        case 32:
            kernelPtr = &kernel_32FLOPS;
            break;
        case 64:
            kernelPtr = &kernel_64FLOPS;
            break;
        case 128:
            kernelPtr = &kernel_128FLOPS;
            break;
        case 256:
            kernelPtr = &kernel_256FLOPS;
            break;
        case 512:
            kernelPtr = &kernel_512FLOPS;
            break;
        case 1024:
            kernelPtr = &kernel_1024FLOPS;
            break;
        default:
            assert (false);
    }

#pragma omp parallel private(id) proc_bind(spread)
    {
        id = omp_get_thread_num();

        // determine thread offsets
        uint64_t nsize = TSIZE / nThreads;
        nsize = nsize & (~(32 - 1));
        nsize = nsize / sizeof(double);
        uint64_t nid = nsize * id;

        // initialize small chunck of buffer within each thread
        initialize(nsize, &buf[nid], 1.0);

        uint64_t nNew = 0;
        for (uint64_t n = 1; n <= nsize;) {
            uint64_t ntrials = nsize / n;
            if (ntrials < 1)
                ntrials = 1;

            for (uint64_t t = 1; t <= ntrials; t = t * 2) { // working set - ntrials
                double startTime, endTime;
                int bytes_per_elem, mem_accesses_per_elem;

#pragma omp barrier

                if (id == 0) {
                    startTime = getTime();
                }

                kernelPtr(n, t, &buf[nid], &bytes_per_elem, &mem_accesses_per_elem);

#pragma omp barrier

#pragma omp master
                if (id == 0) {
                    endTime = getTime();
                    double seconds = (double) (endTime - startTime);
                    uint64_t working_set_size = n * nThreads;
                    uint64_t total_bytes =
                            t * working_set_size * bytes_per_elem * mem_accesses_per_elem;
                    uint64_t total_flops = t * working_set_size * nFlops;

                    results << std::right << std::setw(12) << working_set_size * bytes_per_elem
                            << std::right << std::setw(12) << t
                            << std::right << std::setw(15) << seconds * 1000000
                            << std::right << std::setw(12) << total_bytes
                            << std::right << std::setw(12) << total_flops
                            << std::endl;
                }
            }

            nNew = 1.1 * n;
            if (nNew == n) {
                nNew = n + 1;
            }

            n = nNew;
        } // working set - nsize

    } // parallel region

    free(buf);

    results << std::endl;
    results << "META_DATA" << std::endl;
    results << "FLOPS          " << nFlops << std::endl;
    results << "OPENMP_THREADS " << nThreads << std::endl;
    return env->NewStringUTF(results.str().c_str());
}
