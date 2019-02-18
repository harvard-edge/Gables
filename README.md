# Gables

Welcome to the [_Gables: Roofline Model for Mobile SoCs._](http://research.cs.wisc.edu/multifacet/gables/) The code that is included in here is the code that is used to generate the roofline model for mobile SoCs CPUs/GPUs and DSP. The code has been tested primarily on the following two devices: (1) a Qualcomm 835 Development Kit and (2) Pixel 2/3 smartphones, but it should work on other devices. 

<p align="center"> 
<img src="https://www.intrinsyc.com/wp-content/uploads/2017/05/835-Front-WithShadow.jpg" height="300"> <img src="https://cdn2.gsmarena.com/vv/pics/google/google-pixel-3-4.jpg" height="300">
</p>

The source code that is included here can be built into an Android APK, pushed to the device and then run by a user. You can import the git repo directly from the Android Studio.

# Prerequisites

* You must be familiar with Android Studio to build the APK.
* You have a device based on the Qualcomm chipset. 
* You must grant the app read/write permissions

# Step-by-step

## Install Android Studio

You can download the latest version of Android Studio [here](https://developer.android.com/studio/), and follow the instructions to install.

## Import the GitHub Repo

You can directly import GitHub projects into Android Studio. In the Android Studio window, click VCS -> Checkout -> Git and import this repository address `https://github.com/harvard-edge/Gables`. The GitHub repo will be created as a new project in android studio. Alternatively, you can download the repo to your filesystem and manually import it as a new project into Android Studio. We recommend using the Git repo directly as you can get the latest updates.

## Build the APK

Android Studio should automatically recognize the project is using the Gradle build system. If so you should be able to press `Build>Make Project` and it will attempt to build the project.

We use CMake to build the C++ compenents of the app which interface with the Java code through the JNI. In order to build the app you should have CMake and the Android NDK installed and in your path. Usually Android studio will offer to install these automatically if you attempt to build without these available.

## Install the APK

Once the build has completed successfully, you will be able to press `Run>Run 'app'` in the toolbar and then Android Studio will prompt you to select a device to install the APK on.

We recommend running Gables on a real world device for testing. The device should have [developer options](https://developer.android.com/studio/debug/dev-options) enabled and USB debugging turned on. The device will then appear in the run prompt. Selecting this device will install the APK on it.

If the name of the device doesn't appear correctly in the prompt, check that the device is conencted in the correct file transfer mode (this can usually be changed in a persistent notification on the device itself). The correct file transfer mode varies depending on device but once the correct mode is selected the device name will be visible in the run prompt.

If you do wish to run Gables on an emulator, we currently only support emulator images based on the armeabi-v7a, or arm64-v8a instruction sets. Support for other architectures is coming soon.

## Run the APK

The Gables app icon should now be visible in the launcher of the device in question.

## Generate the Plots

The current version of the app first generates the data, and then processes the data offline to generate the plots. See [step-by-step](step_by_step.md) instructions on how to build, run and generate the offline plots.

# Limitations and Restrictions

The current version of the code has only been tested on devices that use the Qualcomm chipsets. 

If your smartphone or a development bboard that is based on some other chipsets (i.e., not Qualcomm chipsets), modifications may be needed to get the code running. Please contact us for help.

Most users face trouble when it comes to running the code on the DSP. If you are on the phone, the DSP code will likely not work. This is because the DSP kernel library needs to be specially "signed" by Qualcomm to allow access to the DSP. So, the DSP code will not work out of the box. However, if you are on the development platform. The code should work. 

# FAQs

> How do I install the APK?

Please watch the video, or follow the step-by-step instructions on how to build and run the app.

> After running the code, why is the plot not updating?

Real-time plot updating is still work under progress. Please continue to use the offline plot generator. Stay tuned for updates as we are making progress toward releasing the updates soon.

# Authors and Contributors

* Vijay Janapa Reddi (Harvard University)
* Daniel Inge (Harvard University)
* Mark Hill (Univ. of Wisconsin)
* Nikhita Kunati (Univ. of Wisconsin)

Want to contribute something to the app? Please contact us, we would love the help!

# Contact Us

Please email your questions and concerns to vj@eecs.harvard.edu and dinge@college.harvard.edu.

# Credits

The inspiration for the mobile version of the code comes from the [CS Roofline Toolkit
](https://bitbucket.org/berkeleylab/cs-roofline-toolkit). The APK introduces updates and changes to the code, and includes numerous new code additions such as support for GPU OpenGL ES and the DSPs.

