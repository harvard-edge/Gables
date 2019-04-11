###############################################################################
# Sets the minimum version of CMake required to build your jni library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

set(JNILIBS_ROOT ${CMAKE_SOURCE_DIR}/src/main/jniLibs/)

set(CMAKE_PREFIX_PATH /vendor/lib/rfsa/adsp/)

set(CMAKE_VERBOSE_MAKEFILE on)

set(GABLES_FLAGS "-fopenmp -O2 -fno-tree-vectorize -ffast-math -Wdeprecated-register -fno-unroll-loops")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GABLES_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${GABLES_FLAGS}")

###############################################################################
# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add_library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

add_library( # Specifies the name of the library.
             roofline-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/jni/roofline/CPUdriver.cpp
             src/main/jni/roofline/CPUkernel.cpp
             src/main/jni/roofline/GPUdriver.cpp
             src/main/jni/roofline/SOCdriver.cpp
             src/main/jni/roofline/DSPdriver.cpp
             src/main/jni/roofline/Barrier.cpp
             src/main/jni/roofline/Utils.cpp
           )

# Specifies a path to jni header files.
include_directories(src/main/jni/roofline/)

###############################################################################
add_library( app-glue
             STATIC
             ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c )

###############################################################################
# build cpufeatures as a static lib
add_library(cpufeatures
            STATIC
            ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)

###############################################################################
find_library( # Defines the name of the path variable that stores the
              # location of the NDK library.
              egl-lib

              # Specifies the name of the NDK library that
              # CMake needs to locate.
              EGL )

###############################################################################
find_library( # Defines the name of the path variable that stores the
              # location of the NDK library.
              glesV3-lib

              # Specifies the name of the NDK library that
              # CMake needs to locate.
              GLESv3 )

###############################################################################
find_library( # Defines the name of the path variable that stores the
              # location of the NDK library.
              log-lib

              # Specifies the name of the NDK library that
              # CMake needs to locate.
              log )

###############################################################################
target_include_directories(roofline-lib PRIVATE
    ${ANDROID_NDK}/sources/android/cpufeatures)

###############################################################################
add_library(lib_gables SHARED IMPORTED)

set_target_properties(
        lib_gables
        PROPERTIES IMPORTED_LOCATION
        ${JNILIBS_ROOT}/${ANDROID_ABI}/libgables.so)

###############################################################################
# Links your native library against one or more other native libraries.
target_link_libraries( # Specifies the target library.
                       roofline-lib

                       ${egl-lib}
                       ${glesV3-lib}
                       ${log-lib}

                       lib_gables

                       cpufeatures
                       app-glue
                       android)

