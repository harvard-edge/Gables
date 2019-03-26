#include <unistd.h>
#include <stdlib.h>
#include <sched.h>

#include "CPUdriver.hpp"
#include "CPUkernel.hpp"
#include "Utils.hpp"

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_CPUTest(JNIEnv *env, jobject thiz) {
    std::string str = "Hello from CPU Roofline!";
    return env->NewStringUTF(str.c_str());
}

void pushProgress(JNIEnv *env, jobject obj, float percentage) {
    jclass activityClass = env->GetObjectClass(obj);
    jmethodID jniProgressUpdateID = env->GetMethodID(activityClass,
                                                     "jniProgressUpdate", "(F)V");
    assert(jniProgressUpdateID != 0);
    env->CallVoidMethod(obj, jniProgressUpdateID, percentage);
}

CPUKernelPtr CPU_get_kernel_NEON(int nFlops) {
    CPUKernelPtr kptr = NULL;

    switch (nFlops) {
        default:
            LOGE("Unknown kernel type.");
            assert (false);
    }

    return kptr;
}

CPUKernelPtr CPU_get_kernel(int nFlops) {
    CPUKernelPtr kptr = NULL;

    switch (nFlops) {
        case 1:
            kptr = &CPUKernel_1FLOPS;
            break;
        case 2:
            kptr = &CPUKernel_2FLOPS;
            break;
        case 4:
            kptr = &CPUKernel_4FLOPS;
            break;
        case 8:
            kptr = &CPUKernel_8FLOPS;
            break;
        case 16:
            kptr = &CPUKernel_16FLOPS;
            break;
        case 32:
            kptr = &CPUKernel_32FLOPS;
            break;
        case 64:
            kptr = &CPUKernel_64FLOPS;
            break;
        case 128:
            kptr = &CPUKernel_128FLOPS;
            break;
        case 256:
            kptr = &CPUKernel_256FLOPS;
            break;
        case 512:
            kptr = &CPUKernel_512FLOPS;
            break;
        case 1024:
            kptr = &CPUKernel_1024FLOPS;
            break;
        default:
            LOGE("Unknown kernel type.");
            assert (false);
    }

    return kptr;
}

void print_affinity() {
    cpu_set_t mask;
    long nproc, i;

    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        assert(false);
    } else {
        nproc = sysconf(_SC_NPROCESSORS_ONLN);
        LOGI("sched_getaffinity = ");
        for (i = 0; i < nproc; i++) {
            LOGI("%d ", CPU_ISSET(i, &mask));
        }
        printf("\n");
    }
}

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_CPUExecute(JNIEnv *env, jobject obj,
                                           jint memTotal, jint nThreads, jint nFlops,
                                           jboolean neon) {

    int id = 0;

    assert(nThreads >= 1);

    std::ostringstream results;

    uint64_t TSIZE = memTotal;

    float *__restrict__ buf = (float *) memalign(16, TSIZE);
    LOGD("Allocated a buffer of size %lu", TSIZE);
    if (buf == NULL) {
        LOGE("Out of memory!\n");
        std::string str = "Error in CPU Roofline!";
        return env->NewStringUTF(str.c_str());
    }
    memset(buf, 0x0, TSIZE);

    omp_set_dynamic(0);             // Explicitly disable dynamic teams
    omp_set_num_threads(nThreads);  // Use nThreads for all consecutive parallel regions

    CPUKernelPtr kptr;
    if (neon) {
        kptr = CPU_get_kernel_NEON(nFlops);
    } else {
        kptr = CPU_get_kernel(nFlops);
    }

#pragma omp parallel private(id)
    {
        id = omp_get_thread_num();

        // determine thread offsets
        uint64_t nsize = TSIZE / nThreads;
        nsize = nsize & (~(ALIGN - 1));
        LOGD("ThreadID (%d) array nsize = %lu MiB", id, nsize / MiB);

        nsize = nsize / sizeof(float);
        uint64_t nid = nsize * id;
        LOGD("Nsize for each thread (i.e., number of elements) = %lu", nsize);

        uint64_t nNew = 0;
        for (uint64_t n = 1; n <= nsize;) {
            uint64_t t = 10;

            double startTime, endTime;
            int bytes_per_elem, flops_per_elem, mem_accesses_per_elem;

#pragma omp barrier

#pragma omp master
            {
                startTime = getOMPTime();
            }

            kptr(n, t, &buf[nid], &bytes_per_elem, &flops_per_elem, &mem_accesses_per_elem);

#pragma omp barrier

#pragma omp master
            {
                endTime = getOMPTime();
                double total_seconds = endTime - startTime;

                uint64_t working_set_size = n * nThreads;
                uint64_t total_bytes =
                        t * working_set_size * bytes_per_elem * mem_accesses_per_elem;
                uint64_t total_flops = t * working_set_size * nFlops * flops_per_elem;

                results << std::right << std::setw(12) << working_set_size * bytes_per_elem
                        << std::right << std::setw(12) << t
                        << std::right << std::setw(15) << total_seconds * 1000000
                        << std::right << std::setw(12) << total_bytes
                        << std::right << std::setw(12) << total_flops
                        << std::endl;

                double gflops = ((float) total_bytes / GiB) / total_seconds;
                double bwidth = ((float) total_flops / GFLOPS) / total_seconds;

                LOGD("Finished nsize: %llu; trial: %llu (%.5f): [%.4f GiB/s %.4f GFLOPS/s] ", n,
                     t,
                     total_seconds, gflops, bwidth);
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