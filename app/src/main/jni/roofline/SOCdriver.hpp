#ifndef GABLES_SOCDRIVER_H
#define GABLES_SOCDRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <syslog.h>
#include <android/log.h>
#include <jni.h>
#include <omp.h>

#include "Utils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_google_gables_Roofline_SOCTest(JNIEnv *env,
                                                                  jobject thiz);

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_SOCMixer(JNIEnv *env, jobject instance, jint memTotal, jint nFlops,
                                         jfloat cpu_work_frac, jint cpuThreads,
                                         jint gpuWorkGroupSize,
                                         jint gpuWorkItemSize);

#ifdef __cplusplus
}
#endif

#endif //GABLES_SOCDRIVER_H
