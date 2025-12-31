#include <jni.h>
#include <android/log.h>

#define LOG_TAG "FOnline"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonlinetla_MainActivity_nativeInit(JNIEnv* env, jobject /* this */) {
    LOGI("FOnline C++ стартует!");
    // Здесь потом будет инициализация движка FOnline
}
