#ifndef GABLES_GPUDRIVER_H
#define GABLES_GPUDRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <syslog.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <omp.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <GLES3/gl3.h>

//#include "Utils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef GLuint GPUKernelPtr;

bool GPUInitializeOpenGL();
bool GPUShutdownOpenGL();

GPUKernelPtr GPUBuildKernel(int wgThreads, int nFlops);
void GPUConfigureSSBO(uint total_size, uint ntrials);
void GPULaunchKernel(GPUKernelPtr computeKernel, uint wgSize);

JNIEXPORT jboolean JNICALL
Java_com_google_gables_Roofline_GPUInitOpenGL(JNIEnv *env, jobject thiz);

JNIEXPORT jboolean JNICALL
Java_com_google_gables_Roofline_GPUFiniOpenGL(JNIEnv *env, jobject thiz);

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxWorkGroupCount(JNIEnv *env, jobject thiz, jint dim);

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxWorkGroupSize(JNIEnv *env, jobject thiz, jint dim);

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxThreadInnovations(JNIEnv *env, jobject thiz, jint dim);

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_GPUExecute(JNIEnv *env, jobject thiz, jobject assetManager,
                                           jint nGroups, jint nThreads, jint nFlops, jint memTotal);

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_GPUTest(JNIEnv *env, jobject thiz);

#ifdef __cplusplus
}
#endif

#endif //GABLES_GPUDRIVER_H
