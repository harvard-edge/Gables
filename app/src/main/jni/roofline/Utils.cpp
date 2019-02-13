#include <sys/time.h>
#include <omp.h>
#include <string>
#include <sstream>
#include <jni.h>

#include "Utils.hpp"

double getTime() {
    double time;

    struct timeval tm;
    gettimeofday(&tm, NULL);
    time = tm.tv_sec + (tm.tv_usec / 1000000.0);

    return time;
}

double getOMPTime() {
    double time;

    time = omp_get_wtime();
    return time;
}

void initialize(uint64_t nsize,
                double *__restrict__ A,
                double value) {
    uint64_t i;
    for (i = 0; i < nsize; ++i) {
        A[i] = value;
    }
}

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_UtilsSetEnvLibraryPath(JNIEnv *env, jobject thiz,
                                                       jstring envVar,
                                                       jstring envPath) {

    const char *nativeEnvVar = env->GetStringUTFChars(envVar, JNI_FALSE);
    const char *nativeEnvPath = env->GetStringUTFChars(envPath, JNI_FALSE);

    std::stringstream path;
    path << nativeEnvPath << ";/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";

    int ret = setenv(nativeEnvVar, path.str().c_str(), 1 /*override*/) == 0;

    env->ReleaseStringUTFChars(envVar, nativeEnvVar);
    env->ReleaseStringUTFChars(envPath, nativeEnvPath);

    return ret;
}