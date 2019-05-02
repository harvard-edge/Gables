LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := roofline
LOCAL_C_INCLUDES += ./src/main/jni/roofline/inc
LOCAL_CFLAGS += -O3
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
LOCAL_SRC_FILES := ./src/main/jni/roofline/CPUdriver.cpp ./src/main/jni/roofline/CPUkernel.cpp
include $(BUILD_SHARED_LIBRARY)
