#ifndef ERT_DRIVER_H
#define ERT_DRIVER_H

#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <syslog.h>
#include <android/log.h>
#include <omp.h>

#include "Utils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_DSPExecute(JNIEnv *env,
                                           jobject obj,
                                           jint memTotal,
                                           jint nThreads,
                                           jint nFlops,
                                           jboolean hvx);

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_DSPNumThreads(JNIEnv *env,
                                              jobject thiz,
                                              jboolean hvx);

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_DSPTest(JNIEnv *env,
                                        jobject thiz);

#ifdef __cplusplus
}
#endif

#endif