package com.google.gables;

import android.content.res.AssetManager;
import android.util.Log;

public class Roofline {
    private static final String TAG = DSPRoofline.class.getName();

    /* ================================================================================= */
    /* Library providing functionality                                                   */
    /* ================================================================================= */
    static {
        String filename;

        filename = "roofline";
        try {
            System.loadLibrary(filename);
            Log.i(TAG, filename + " successfully loaded!");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, filename + " NOT successfully loaded: " + e);
            Log.e(TAG, "Terminating application because library not found.");
            android.os.Process.killProcess(android.os.Process.myPid());
        }

//        filename = "gables";
//        try {
//            System.loadLibrary(filename);
//            Log.i(TAG, filename + " successfully loaded!");
//        } catch (UnsatisfiedLinkError e) {
//            Log.e(TAG, filename + " NOT successfully loaded: " + e);
//            Log.e(TAG, "Terminating application because library not found.");
//            android.os.Process.killProcess(android.os.Process.myPid());
//        }
    }

    /* ================================================================================= */
    /* GPU code                                                                          */
    /* ================================================================================= */

    public static boolean GPU_Initialize() {
        return new Roofline().GPUInitOpenGL();
    }

    public static boolean GPU_Finish() {
        return new Roofline().GPUFiniOpenGL();
    }

    public static int GPU_MaxWorkGroupSize(int dim) {
        return new Roofline().GPUMaxWorkGroupSize(dim);
    }

    public static int GPU_MaxWorkGroupCount(int dim) {
        return new Roofline().GPUMaxWorkGroupCount(dim);
    }

    public static int GPU_MaxThreadInnovations(int dim) {
        return new Roofline().GPUMaxThreadInnovations(dim);
    }

    public static String GPU_Execute(AssetManager mgr, int nGroups, int nThreads, int nFlops, int memSize) {
        return new Roofline().GPUExecute(mgr, nGroups, nThreads, nFlops, memSize);
    }

    public static String CPU_Execute(int memTotal, int nThreads, int nFlops, boolean neon) {
        return new Roofline().CPUExecute(memTotal, nThreads, nFlops, neon);
    }

    public static String DSP_Execute(int memTotal, int nThreads, int nFlops, boolean hvx) {
        return new Roofline().DSPExecute(memTotal, nThreads, nFlops, hvx);
    }

    public static int DSP_NumThreads(boolean hvx) {
        return new Roofline().DSPNumThreads(hvx);
    }

    public static String SOC_Mixer(int memTotal, int nFlops, float cpuWorkFraction, int cpuThreads, int gpuWorkGroupSize, int gpuWorkItemSize) {
        return new Roofline().SOCMixer(memTotal, nFlops, cpuWorkFraction, cpuThreads, gpuWorkGroupSize, gpuWorkItemSize);
    }

    public static int Utils_SetEnvLibraryPath(String var, String path) {
        return new Roofline().UtilsSetEnvLibraryPath(var, path);
    }

    /* ================================================================================= */
    /* GPU code                                                                          */
    /* ================================================================================= */
    private native int GPUMaxWorkGroupSize(int dim);

    private native int GPUMaxWorkGroupCount(int dim);

    private native int GPUMaxThreadInnovations(int dim);

    private native boolean GPUInitOpenGL();

    private native boolean GPUFiniOpenGL();

    private native String GPUExecute(AssetManager mgr, int nGroups, int nThreads, int nFlops, int memSize);

    /* ================================================================================= */
    /* CPU code                                                                          */
    /* ================================================================================= */
    private native String CPUExecute(int maxMem, int nThreads, int nFlops, boolean neon);

    /* ================================================================================= */
    /* DSP code                                                                          */
    /* ================================================================================= */
    private native String DSPExecute(int maxMem, int nThreads, int nFlops, boolean hvx);

    private native int DSPNumThreads(boolean hvx);

    /* ================================================================================= */
    /* SOC code                                                                          */
    /* ================================================================================= */
    private native String SOCMixer(int memTotal, int nFlops, float cpuWorkFraction, int cpuThreads, int gpuWorkGroupSize, int gpuWorkItemSize);

    /* ================================================================================= */
    /* Utils code
    /* ================================================================================= */
    private native int UtilsSetEnvLibraryPath(String var, String path);
}
