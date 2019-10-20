LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := roofline
LOCAL_C_INCLUDES += ./src/main/jni/roofline/inc
LOCAL_CFLAGS += -O3
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
LOCAL_SRC_FILES := ./src/main/jni/roofline/CPUdriver.cpp ./src/main/jni/roofline/CPUkernel.cpp ./src/main/jni/roofline/GPUdriver.cpp ./src/main/jni/roofline/Utils.cpp
LOCAL_LDLIBS := -lGLESv3 -landroid -lEGL -llog
include $(BUILD_SHARED_LIBRARY)

