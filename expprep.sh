#!/bin/bash -x

###############################################################################
# ADB settings
###############################################################################
adb root

###############################################################################
# CPU, GPU and SYSTEM settings
###############################################################################

# From Qualcomm® Snapdragon™ Mobile Platform OpenCL General Programming and Optimization performance mode
adb shell "echo 0 > /sys/class/kgsl/kgsl-3d0/min_pwrlevel"
adb shell "echo performance > /sys/class/kgsl/kgsl-3d0/devfreq/governor"

adb shell "echo performance > /sys/class/devfreq/soc\:qcom,cpubw/governor"
adb shell "echo performance > /sys/class/devfreq/soc\:qcom,gpubw/governor"
adb shell "echo performance > /sys/class/devfreq/soc\:qcom,mincpubw/governor"
adb shell "echo performance > /sys/class/devfreq/soc\:qcom,memlat-cpu0/governor"
adb shell "echo performance > /sys/class/devfreq/soc\:qcom,memlat-cpu4/governor"

adb shell "echo 1 > /sys/devices/system/cpu/cpu0/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu4/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu5/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu6/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu7/online"

#adb shell stop thermald
#adb shell stop mpdecision
#adb shell stop vendor.perfd
#adb shell stop performanced
#adb shell stop thermal-engine
#adb shell stop vendor.thermal-engine

#adb shell "cat /sys/class/kgsl/kgsl-3d0/gpuclk"
#adb shell "echo 1 > /sys/class/kgsl/kgsl-3d0/force_clk_on"
#adb shell "echo 1 > /sys/class/kgsl/kgsl-3d0/force_bus_on"
#adb shell "echo 1 > /sys/class/kgsl/kgsl-3d0/force_rail_on"
#adb shell "echo 1000000 > /sys/class/kgsl/kgsl-3d0/idle_timer"

adb shell "echo 0 > /sys/class/kgsl/kgsl-3d0/min_pwrlevel"
adb shell "echo 0 > /sys/class/kgsl/kgsl-3d0/max_pwrlevel"

# disable "long instruction buffer detect" which kills(!) long running GPU kernels
GPU_LONGIB_DETECT="/sys/class/kgsl/kgsl-3d0/ft_long_ib_detect"
adb shell "echo 0 > $GPU_LONGIB_DETECT"

# disable sepolicy to allow access to DSP
adb shell setenforce 0
