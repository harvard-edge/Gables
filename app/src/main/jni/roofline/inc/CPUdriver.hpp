#ifndef ERT_DRIVER_H
#define ERT_DRIVER_H
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <syslog.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <assert.h>

#include <string>

#include <omp.h>


#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_CPUExecute(JNIEnv *env,
                                           jobject obj,
                                           jint memTotal,
                                           jint nThreads,
                                           jint nFlops,
                                           jboolean neon);
#ifdef __cplusplus
}
#endif

#endif
