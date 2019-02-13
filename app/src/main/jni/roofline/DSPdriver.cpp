#include "DSPdriver.hpp"

#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>

#ifdef __cplusplus
extern "C" {
#endif

extern int gables_hvx_threads();
extern int gables_hex_threads();

#ifdef __cplusplus
}
#endif


JNIEXPORT jstring JNICALL Java_com_google_gables_Roofline_DSPTest(JNIEnv *env,
                                                                  jobject thiz) {
    std::string str = "Hello from CPU Roofline!";
    return env->NewStringUTF(str.c_str());
}

JNIEXPORT jint JNICALL Java_com_google_gables_Roofline_DSPNumThreads(JNIEnv *env,
                                                                     jobject thiz,
                                                                     jboolean hvx) {
    int thCnt = 0;

    if (hvx) {
        thCnt = gables_hvx_threads();
        LOGI("DSP result for HVX thread count is %d", thCnt);
    } else {
        thCnt = gables_hex_threads();
        LOGI("DSP result for HEX thread count is %d", thCnt);
    }

    return thCnt;
}

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_DSPExecute(JNIEnv *env, jobject thiz,
                                           jint memTotal, jint nThreads,
                                           jint nFlops, jboolean hvx) {
    std::ostringstream results;

    //gables_hex_driver(memTotal, nThreads, nFlops, hvx);

    return env->NewStringUTF(results.str().c_str());
}