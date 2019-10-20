#ifndef GABLES_UTILS_HPP
#define GABLES_UTILS_HPP

#define KiB     (1024)
#define MiB     (KiB * KiB)
#define GiB     (MiB * KiB)

#define ALIGN   (32)

#define GFLOPS  (1000000000)

#define LOG_TAG ("JNI")

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

#ifdef __cplusplus
extern "C" {
#endif

double getTime();

double getOMPTime();

//void initialize(uint64_t nsize,
//                float *__restrict__ array,
//                float value);

JNIEXPORT jint JNICALL
Java_com_google_gables_Roofline_UtilsSetEnvLibraryPath(JNIEnv *env, jobject thiz,
                                                       jstring envVar,
                                                       jstring envPath);

#ifdef __cplusplus
}
#endif

#endif //GABLES_UTILS_HPP