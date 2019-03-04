LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := gables
LOCAL_C_INCLUDES += ./inc
LOCAL_CFLAGS += -O3
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
LOCAL_SRC_FILES := ./src/CPUdriver.cpp ./src/CPUkernel.cpp
include $(BUILD_EXECUTABLE)
