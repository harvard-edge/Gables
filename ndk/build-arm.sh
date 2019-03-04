#!/bin/bash 

export NDK_PROJECT_PATH=.
ndk-build NDK_APPLICATION_MK=./Application.mk
