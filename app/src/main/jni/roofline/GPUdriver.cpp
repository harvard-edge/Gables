#include "GPUdriver.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "Utils.hpp"

#define GPUROOFLINE_MIN_NSIZE (1024)

static const char gComputeShader[] =
        "#version 320 es\n"
        "\n"
        "#define KERNEL1(a,b,c)   ((a) = (b) + (c))\n"
        "#define KERNEL2(a,b,c)   ((a) = (a)*(b) + (c))\n"
        "\n"
        "#define REP2(S)        S ;        S\n"
        "#define REP4(S)   REP2(S);   REP2(S)\n"
        "#define REP8(S)   REP4(S);   REP4(S)\n"
        "#define REP16(S)  REP8(S);   REP8(S)\n"
        "#define REP32(S)  REP16(S);  REP16(S)\n"
        "#define REP64(S)  REP32(S);  REP32(S)\n"
        "#define REP128(S) REP64(S);  REP64(S)\n"
        "#define REP256(S) REP128(S); REP128(S)\n"
        "#define REP512(S) REP256(S); REP256(S)\n"
        "#define REP1024(S) REP512(S); REP512(S)\n"
        "#define REP2048(S) REP1024(S); REP1024(S)\n"
        "#define REP4096(S) REP2048(S); REP2048(S)\n"
        "#define REP8192(S) REP4096(S); REP4096(S)\n"
        "#define REP16384(S) REP8192(S); REP8192(S)\n"
        "#define REP32768(S) REP16384(S); REP16384(S)\n"
        "\n"
        "layout(local_size_x = XXX_LAYOUT_SIZE_XXX, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout(binding = 0) readonly buffer Params {\n"
        "    uint data[];\n"
        "} params;\n"
        "layout(binding = 1) buffer IArray {\n"
        "    float data[];\n"
        "} ibuff;\n"
        "layout(binding = 2) writeonly buffer OArray {\n"
        "    uint data[];\n"
        "} retval;\n"
        "\n"
        "void main()\n"
        "{\n"
        "  uint wsssize = params.data[0];\n"
        "  uint ntrials = params.data[1];\n"
        "\n"
        "  uint total_thr = gl_NumWorkGroups.x * gl_WorkGroupSize.x;\n"
        "  uint elem_per_thr = (wsssize + (total_thr - 1u)) / total_thr;\n"
        "\n"
        "  uint start_idx = gl_GlobalInvocationID.x;\n"
        "  uint end_idx = start_idx + elem_per_thr * total_thr;\n"
        "\n"
        "  uint stride_idx = total_thr;\n"
        "\n"
        "  if (start_idx > wsssize)\n"
        "    start_idx = wsssize;\n"
        "  if (end_idx > wsssize)\n"
        "    end_idx = wsssize;\n"
        "\n"
        "  float alpha = 0.5;\n"
        "  for (uint j = 0u; j < ntrials; ++j) {\n"
        "    for (uint i = start_idx; i < end_idx; i += stride_idx) {\n"
        "      float beta = 0.8;\n"
        "      XXX_KERNEL_FLOPS_PREFIX_XXX(beta, ibuff.data[i], alpha)XXX_KERNEL_FLOPS_POSTFIX_XXX;\n"
        "      retval.data[3] = uint(beta);\n"
        "    }\n"
        "  }\n"
        "\n"
        "  retval.data[0] = 4u;\n"
        "  retval.data[1] = 2u;\n"
        "  retval.data[2] = 1u;\n"
        "  retval.data[3] = 0u;\n"
        "}\n";

//                 "      retval.data[3] = uint(beta);\n"
//                "      ibuff.data[i] = beta;\n"

static EGLDisplay gDisplay;
static EGLContext gContext;

#define CHECK() \
{\
    GLenum err; \
    while((err = glGetError()) != GL_NO_ERROR)\
    {\
        LOGE("glGetError returns %d\n", err); \
        assert(false);\
    }\
}

#define CONTEXT_NOT_ALIVE() \
{\
  if (eglGetCurrentContext() != EGL_NO_CONTEXT) { \
    LOGE("There is no context to close-up. Did you initialize?");\
    assert(false);\
  }\
}

#define CONTEXT_ALIVE() \
{\
  if (eglGetCurrentContext() == EGL_NO_CONTEXT) { \
    LOGE("There is no context to close-up. Did you initialize?");\
    assert(false);\
  }\
}

GLuint loadShader(GLenum shaderType, const char *pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
            assert(false);
        }
    } else {
        assert(false);
    }
    return shader;
}

GLuint GPUCompileKernel(const char *pComputeSource) {
    GLuint computeShader = loadShader(GL_COMPUTE_SHADER, pComputeSource);
    if (!computeShader) {
        assert(false);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, computeShader);
        glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
            assert(false);
        }
        // done linking so delete
        glDetachShader(program, computeShader);
        glDeleteShader(computeShader);
    } else {
        assert(false);
    }
    return program;
}

std::string getComputeShaderKernelTemplate(AAssetManager *mgr, const char kernelFilename[]) {

    assert(NULL != mgr);

    char *fileBuffer = NULL;

    AAsset *testAsset = AAssetManager_open(mgr, kernelFilename, AASSET_MODE_UNKNOWN);
    if (testAsset) {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        fileBuffer = (char *) malloc(assetLength + 1);
        AAsset_read(testAsset, fileBuffer, assetLength);
        fileBuffer[assetLength] = 0;

        //LOGD("The value is %s", fileBuffer);

        AAsset_close(testAsset);
    } else {
        LOGD("Cannot open asset file %s for loading GPU kernel", kernelFilename);
        assert(false);
    }

    std::string fileString = std::string(fileBuffer);

    delete[] fileBuffer;

    return fileString;
}

bool replace(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }

    str.replace(start_pos, from.length(), to);

    return true;
}

std::string
GPUMakeKernelSource(const std::string gComputeShaderKernelTemplate,
                    const uint64_t nThreads,
                    const uint64_t nFlops) {

    // create a local copy that we can modify
    std::string gComputeShader = gComputeShaderKernelTemplate;

    // set the number of shader threads
    std::string k_layoutsize = "XXX_LAYOUT_SIZE_XXX";
    std::string v_layoutsize = std::to_string(nThreads);

    if (!replace(gComputeShader, k_layoutsize, v_layoutsize)) {
        LOGE("Failed to replace layout size in shader template");
        assert(false);
    }

    //LOGD("Original replacing XXX_LAYOUT_SIZE_XXX:\n%s", gComputeShaderKernelTemplate.c_str());

    std::string v_flops_prefix;
    std::string v_flops_postfix;
    switch (nFlops) {
        case 1:
            v_flops_prefix = "KERNEL1";
            v_flops_postfix = "";
            break;
        case 2:
            v_flops_prefix = "KERNEL2";
            v_flops_postfix = "";
            break;
        case 4:
            v_flops_prefix = "REP2(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 8:
            v_flops_prefix = "REP4(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 16:
            v_flops_prefix = "REP8(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 32:
            v_flops_prefix = "REP16(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 64:
            v_flops_prefix = "REP32(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 128:
            v_flops_prefix = "REP64(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 256:
            v_flops_prefix = "REP128(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 512:
            v_flops_prefix = "REP256(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 1024:
            v_flops_prefix = "REP512(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 2048:
            v_flops_prefix = "REP1024(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 4096:
            v_flops_prefix = "REP2048(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 8192:
            v_flops_prefix = "REP4096(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 16384:
            v_flops_prefix = "REP8192(KERNEL2";
            v_flops_postfix = ")";
            break;
        case 32768:
            v_flops_prefix = "REP16384(KERNEL2";
            v_flops_postfix = ")";
            break;
        default:
            assert(false);
    }

    std::string k_flops_prefix = "XXX_KERNEL_FLOPS_PREFIX_XXX";
    std::string k_flops_postfix = "XXX_KERNEL_FLOPS_POSTFIX_XXX";

    if (!replace(gComputeShader, k_flops_prefix, v_flops_prefix)) {
        LOGE("Failed to find PREFIX replacement in shader template.");
        assert(false);
    }
    if (!replace(gComputeShader, k_flops_postfix, v_flops_postfix)) {
        LOGE("Failed to find POSTFIX replacement in shader template.");
        assert(false);
    }

    //LOGD("After replacing XXX_KERNEL_FLOPS_XXX:\n%s", gComputeShader.c_str());
    LOGD("Generated GPU kernel source code for %d wgThreads @ %d FLOPS", nThreads, nFlops);

    return gComputeShader;
}

void printDbgOpenGLInfo() {
    int work_grp_size[3], work_grp_inv;

    // maximum global work group (total work in a dispatch)
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_size[2]);
    LOGD("max global (total) work group size x:%i y:%i z:%i\n", work_grp_size[0],
         work_grp_size[1], work_grp_size[2]);

    // maximum local work group (one shader's slice)
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    LOGD("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
         work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    // maximum compute shader invocations (x * y * z)
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    LOGD("max computer shader invocations %i\n", work_grp_inv);
}


JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_GPUTest(JNIEnv *env, jobject thiz) {
    std::string str = "Hello from GPU Roofline!";
    return env->NewStringUTF(str.c_str());
}

void dumpProgram(GLuint program) {

    GLint len;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);
    CHECK();

    char *binary = new char[len];
    GLenum binaryFormat;

    glGetProgramBinary(program, len, NULL, &binaryFormat, &binary[0]);
    CHECK();

    std::string path = "/data/data/com.google.gables/shader.bin";

    std::ofstream ofile(path.c_str());
    assert(ofile.is_open());

    ofile.write(reinterpret_cast<char *>(&binary), len);

    free(binary);
    ofile.close();
}

bool GPUShutdownOpenGL() {

    CONTEXT_ALIVE();

    eglMakeCurrent(gDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    EGLBoolean result = eglDestroyContext(gDisplay, gContext);
    assert(result == EGL_TRUE);

    result = eglTerminate(gDisplay);
    assert(result == EGL_TRUE);

    gDisplay = EGL_NO_DISPLAY;
    gContext = EGL_NO_CONTEXT;

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_gables_Roofline_GPUFiniOpenGL(JNIEnv *env, jobject thiz) {

    CONTEXT_ALIVE();

    // wrapper since it doesn't need env/thiz
    GPUShutdownOpenGL();
    return JNI_TRUE;
}

int getContextVersion(EGLDisplay d, EGLContext c) {

    CONTEXT_ALIVE();

    int values[1];
    eglQueryContext(d, c, EGL_CONTEXT_CLIENT_VERSION, values);

    LOGD("EGLContext created, client version: %d", values[0]);

    return values[0];
}

bool GPUInitializeOpenGL() {

    CONTEXT_NOT_ALIVE();

    gDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay returned EGL_NO_DISPLAY.\n");
        return JNI_FALSE;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean returnValue = eglInitialize(gDisplay, &majorVersion, &minorVersion);
    if (returnValue != EGL_TRUE) {
        LOGE("eglInitialize failed\n");
        return JNI_FALSE;
    }
    LOGD("EGL majorVer. = %d and minorVer. = %d", majorVersion, minorVersion);

    EGLConfig cfg;
    EGLint count;
    EGLint s_configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_NONE};
    if (eglChooseConfig(gDisplay, s_configAttribs, &cfg, 1, &count) == EGL_FALSE) {
        LOGE("eglChooseConfig failed\n");
        return JNI_FALSE;
    }

    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    gContext = eglCreateContext(gDisplay, cfg, EGL_NO_CONTEXT, context_attribs);
    if (gContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed\n");
        return JNI_FALSE;
    }
    returnValue = eglMakeCurrent(gDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContext);
    if (returnValue != EGL_TRUE) {
        LOGE("eglMakeCurrent failed returned %d\n", returnValue);
        return JNI_FALSE;
    }

    assert(getContextVersion(gDisplay, gContext) == 3);

    printDbgOpenGLInfo();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_gables_Roofline_GPUInitOpenGL(JNIEnv *env, jobject thiz) {

    CONTEXT_NOT_ALIVE();

    // wrapper since this doesn't really need env/thiz
    GPUInitializeOpenGL();

    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxThreadInnovations(JNIEnv *env, jobject thiz, jint dim) {

    CONTEXT_ALIVE();

    GLint result;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &result);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxWorkGroupCount(JNIEnv *env, jobject thiz, jint dim) {

    CONTEXT_ALIVE();

    GLint result;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, dim, &result);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_GPUMaxWorkGroupSize(JNIEnv *env, jobject thiz, jint dim) {

    CONTEXT_ALIVE();

    GLint result;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, dim, &result);

    return result;
}

void CheckLongIB() {
    // fixme, this requires root privileges to modify so best we can do is read
    // fixme, need to update the code so that we can read it cleanly
    std::ifstream inputFile("/sys/class/kgsl/kgsl-3d0/ft_long_ib_detect");
}

JNIEXPORT jstring JNICALL
Java_com_google_gables_Roofline_GPUExecute(JNIEnv *env, jobject thiz, jobject assetManager,
                                           jint nGroups, jint nThreads,
                                           jint nFlops, jint memTotal) {

#if DEBUG
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    const std::string gComputeShaderKernelTemplate = getComputeShaderKernelTemplate(mgr,
                                                                                    "GPUkernel_template.comp");
#endif

    CONTEXT_ALIVE();

    // ============================================================================== //
    // Setup the source code and compile it to run
    // ============================================================================== //
    std::string srcCode = GPUMakeKernelSource(gComputeShader, nThreads, nFlops);
    GLuint computeKernel = GPUCompileKernel(srcCode.c_str());

    // ============================================================================== //
    // 1. setup the kernel input parameters
    // ============================================================================== //
    GLuint paramSSbo;
    const GLuint paramArraySize = 2;
    uint paramArray[paramArraySize];
    memset(paramArray, 0, paramArraySize * sizeof(uint));

    glGenBuffers(1, &paramSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, paramSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, paramArraySize * sizeof(uint), paramArray,
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, paramSSbo);
    CHECK();

    // ============================================================================== //
    // 2. setup the actual array to process
    // ============================================================================== //
    GLuint ibuffSSbo;
    glGenBuffers(1, &ibuffSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ibuffSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, memTotal, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ibuffSSbo);
    CHECK();

    // ============================================================================== //
    // 3. create a buffer for the kernel to write out some output
    // ============================================================================== //
    GLuint obuffSSbo;
    const GLuint retvalArraySize = 4;
    glGenBuffers(1, &obuffSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, obuffSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, retvalArraySize * sizeof(uint), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, obuffSSbo);
    CHECK();

    // tell openGL here is the code to run
    glUseProgram(computeKernel);
    CHECK();

    // ============================================================================== //
    // determine iteration sweep counts
    // ============================================================================== //
    uint nsize = (uint) memTotal;
    nsize = nsize & (~(ALIGN - 1));
    nsize = nsize / sizeof(float);
    LOGD("Array size = %d MiB", memTotal / MiB);
    LOGD("Nsize (number of elements) = %d", nsize);

    // ============================================================================== //
    // get ready to run the code now
    // ============================================================================== //
    uint64_t nNew = 0;
    std::ostringstream results;
    for (uint n = GPUROOFLINE_MIN_NSIZE; n <= nsize;) {
        uint ntrials = nsize / n;
        if (ntrials < 1)
            ntrials = 1;

        for (uint t = 1; t <= ntrials; t *= 2) {

            // ============================================================================== //
            // update the loop iteration parameters for the gpu to know
            // ============================================================================== //
            paramArray[0] = n;
            paramArray[1] = t;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, paramSSbo);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, paramArraySize * sizeof(uint), paramArray);
            CHECK();

            // ============================================================================== //
            // run the test
            // ============================================================================== //
            double startTime = getTime();
            glDispatchCompute(nGroups, 1, 1);
            CHECK(); // if it fails here, check to see if long_ib is enabled
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glFinish();
            double endTime = getTime();
            CHECK();
            LOGD("Dispatched with %d groups", nGroups);

            // ============================================================================== //
            // get the output from the kernel side of the world
            // ============================================================================== //
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, obuffSSbo);
            uint *pOut = (uint *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                   retvalArraySize * sizeof(uint), GL_MAP_READ_BIT);
            assert(pOut);
            int bytes_per_elem = pOut[0];
            int mem_accesses_per_elem = pOut[1];
            int flops_per_elem = pOut[2];
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // ============================================================================== //
            // time to compute details
            // ============================================================================== //
            double total_seconds = endTime - startTime;
            uint64_t working_set_size = n;
            uint64_t total_bytes = t * working_set_size * bytes_per_elem * mem_accesses_per_elem;
            uint64_t total_flops = t * working_set_size * nFlops * flops_per_elem;

            results << std::right << std::setw(12) << working_set_size * bytes_per_elem
                    << std::right << std::setw(12) << t
                    << std::right << std::setw(15) << total_seconds * 1000000
                    << std::right << std::setw(12) << total_bytes
                    << std::right << std::setw(12) << total_flops
                    << std::endl;

            double bwidth = ((float) total_bytes / GiB) / total_seconds;
            double gflops = ((float) total_flops / GFLOPS) / total_seconds;

            LOGD("Finished nsize: %d; trial: %d (%.5f): [%.4f GiB/s %.4f GFLOPS/s] ", n, t,
                 total_seconds, bwidth, gflops);
        }

        nNew = 1.1 * n;
        if (nNew == n) {
            nNew = n + 1;
        }
        n = nNew;
    }

    // these should not be deleted till now,
    // otherwise GL releases the bindings
    glDeleteBuffers(1, &paramSSbo);
    glDeleteBuffers(1, &ibuffSSbo);
    glDeleteBuffers(1, &obuffSSbo);
    CHECK();

    glDeleteProgram(computeKernel);
    CHECK();

    results << std::endl;
    results << "META_DATA" << std::endl;
    results << "FLOPS          " << nFlops << std::endl;
    results << "GPU_BLOCKS     " << nGroups << std::endl;
    results << "GPU_THREADS    " << nThreads << std::endl;

    return env->NewStringUTF(results.str().c_str());
}

GPUKernelPtr GPUBuildKernel(int wgThreads, int nFlops) {

    // first generate the appropriate source code
    std::string srcCode = GPUMakeKernelSource(gComputeShader, wgThreads, nFlops);

    // now compile the source code
    GLuint computeKernel = GPUCompileKernel(srcCode.c_str());

    // return the compiled code handle
    return computeKernel;
}

void GPUConfigureSSBO(uint total_size, uint ntrials) {
    CONTEXT_ALIVE();

    // ============================================================================== //
    // setup the kernel input parameters
    // ============================================================================== //
    GLuint paramSSbo;
    const GLuint paramArraySize = 2;
    uint paramArray[paramArraySize];
    memset(paramArray, 0, paramArraySize * sizeof(uint));

    paramArray[0] = total_size / sizeof(float);
    paramArray[1] = ntrials;

    glGenBuffers(1, &paramSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, paramSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, paramArraySize * sizeof(uint), paramArray,
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, paramSSbo);
    CHECK();

    // ============================================================================== //
    // create a buffer for the GPU to read
    // ============================================================================== //
    GLuint ibuffSSbo;
    glGenBuffers(1, &ibuffSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ibuffSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, total_size /* already includes bytes */, NULL,
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ibuffSSbo);
    CHECK();

    // ============================================================================== //
    // create a buffer for the kernel to write out some output
    // ============================================================================== //
    GLuint obuffSSbo;
    const GLuint retvalArraySize = 3;
    glGenBuffers(1, &obuffSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, obuffSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, retvalArraySize * sizeof(uint), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, obuffSSbo);
    CHECK();
}

void GPULaunchKernel(GPUKernelPtr computeKernel, uint wgSize) {
    CONTEXT_ALIVE();

    glUseProgram(computeKernel);
    CHECK();

    double startTime = getTime();
    glDispatchCompute(wgSize, 1, 1);
    CHECK(); // if it fails here, check to see if long_ib is enabled
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glFinish();
    double endTime = getTime();
    CHECK();
    LOGD("Internal Execution time = %f seconds w/ %d wgSize", endTime - startTime, wgSize);
}