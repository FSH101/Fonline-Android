#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#define LOG_TAG "FONLINE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
extern "C" bool AndroidClientInit();
extern "C" void AndroidClientFrame();
extern "C" void AndroidClientShutdown();
#endif

namespace {
    std::mutex gMutex;
    ANativeWindow* gWindow = nullptr;
    int gW = 0;
    int gH = 0;

    std::atomic<bool> gRunning{false};
    std::thread gRenderThread;

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
    std::atomic<bool> gEngineReady{false};
#endif

    std::atomic<int> gTouchCount{0};
    std::atomic<float> gLastX{0};
    std::atomic<float> gLastY{0};

    jobject gAssetMgrObj = nullptr;
    AAssetManager* gAssetMgr = nullptr;
    std::string gFilesDir;

    void stopRenderThreadLocked() {
        gRunning.store(false);
        if (gRenderThread.joinable()) {
            gRenderThread.join();
        }
    }

    void releaseWindowLocked() {
        if (gWindow) {
            ANativeWindow_release(gWindow);
            gWindow = nullptr;
        }
        gW = gH = 0;
    }

    void startRenderThreadLocked() {
        stopRenderThreadLocked();
        if (!gWindow) return;

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
        if (!gEngineReady.load()) {
            LOGI("InitApp: starting AndroidClientInit");
            gEngineReady.store(AndroidClientInit());
            LOGI("InitApp result: %d", static_cast<int>(gEngineReady.load()));
        }
#endif

        gRunning.store(true);
        gRenderThread = std::thread([] {
            using namespace std::chrono_literals;
            int frame = 0;

            while (gRunning.load()) {
                ANativeWindow* wnd = nullptr;
                int w = 0, h = 0;
                bool ready = false;

                {
                    std::lock_guard<std::mutex> lk(gMutex);
                    wnd = gWindow;
                    w = gW;
                    h = gH;

                    if (wnd && w > 0 && h > 0) {
                        ANativeWindow_acquire(wnd);
                        ready = true;
                    }
                }

                // ✅ FIX: no goto (C++ forbids jumping over variable initialization)
                if (!ready) {
                    std::this_thread::sleep_for(16ms);
                    continue;
                }

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
                if (gEngineReady.load()) {
                    LOGI("BeginFrame %d", frame);
                    AndroidClientFrame();
                }
#endif

                ANativeWindow_Buffer buffer{};
                if (ANativeWindow_lock(wnd, &buffer, nullptr) == 0) {
                    const int touches = gTouchCount.load();
                    const float lx = gLastX.load();
                    const float ly = gLastY.load();

                    auto* pixels = static_cast<uint32_t*>(buffer.bits);
                    const int stride = buffer.stride;

                    const uint8_t baseR = static_cast<uint8_t>((frame * 2) & 0xFF);
                    const uint8_t baseG = static_cast<uint8_t>((touches * 40) & 0xFF);
                    const uint8_t baseB = static_cast<uint8_t>((static_cast<int>(lx + ly)) & 0xFF);

                    for (int y = 0; y < buffer.height; y++) {
                        uint32_t* row = pixels + y * stride;
                        for (int x = 0; x < buffer.width; x++) {
                            const uint8_t r = static_cast<uint8_t>((baseR + x) & 0xFF);
                            const uint8_t g = static_cast<uint8_t>((baseG + y) & 0xFF);
                            const uint8_t b = baseB;
                            row[x] = 0xFF000000u
                                   | (static_cast<uint32_t>(r) << 16)
                                   | (static_cast<uint32_t>(g) << 8)
                                   | (static_cast<uint32_t>(b));
                        }
                    }

                    // Crosshair at last touch
                    const int cx = static_cast<int>(lx);
                    const int cy = static_cast<int>(ly);

                    for (int dx = -20; dx <= 20; dx++) {
                        const int px = cx + dx, py = cy;
                        if (px >= 0 && px < buffer.width && py >= 0 && py < buffer.height) {
                            pixels[py * stride + px] = 0xFFFFFFFFu;
                        }
                    }
                    for (int dy = -20; dy <= 20; dy++) {
                        const int px = cx, py = cy + dy;
                        if (px >= 0 && px < buffer.width && py >= 0 && py < buffer.height) {
                            pixels[py * stride + px] = 0xFFFFFFFFu;
                        }
                    }

                    ANativeWindow_unlockAndPost(wnd);
                } else {
                    LOGE("ANativeWindow_lock failed");
                }

                ANativeWindow_release(wnd);

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
                if (gEngineReady.load()) {
                    LOGI("EndFrame %d", frame);
                }
#endif

                frame++;
                std::this_thread::sleep_for(16ms); // ~60 fps
            }
        });
    }
}

// JNI: com.example.fonline.NativeBridge
extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeInit(JNIEnv* env, jobject, jobject assets, jstring filesDirPath) {
    std::lock_guard<std::mutex> lk(gMutex);

    if (gAssetMgrObj) {
        env->DeleteGlobalRef(gAssetMgrObj);
        gAssetMgrObj = nullptr;
        gAssetMgr = nullptr;
    }

    gAssetMgrObj = env->NewGlobalRef(assets);
    gAssetMgr = AAssetManager_fromJava(env, assets);

    const char* cpath = env->GetStringUTFChars(filesDirPath, nullptr);
    gFilesDir = cpath ? cpath : "";
    env->ReleaseStringUTFChars(filesDirPath, cpath);

    LOGI("nativeInit OK. filesDir=%s assetMgr=%p", gFilesDir.c_str(), gAssetMgr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnSurfaceCreated(JNIEnv* env, jobject, jobject surface) {
    std::lock_guard<std::mutex> lk(gMutex);

    stopRenderThreadLocked();
    releaseWindowLocked();

    gWindow = ANativeWindow_fromSurface(env, surface);
    if (!gWindow) {
        LOGE("ANativeWindow_fromSurface returned null");
        return;
    }

    ANativeWindow_acquire(gWindow);
    ANativeWindow_setBuffersGeometry(gWindow, 0, 0, WINDOW_FORMAT_RGBA_8888);
    LOGI("Surface created. window=%p", gWindow);

    startRenderThreadLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnSurfaceChanged(JNIEnv*, jobject, jint width, jint height) {
    std::lock_guard<std::mutex> lk(gMutex);
    gW = width;
    gH = height;
    LOGI("Surface changed: %dx%d", gW, gH);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnSurfaceDestroyed(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lk(gMutex);
    LOGI("Surface destroyed");

    stopRenderThreadLocked();
    releaseWindowLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnResume(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lk(gMutex);
    LOGI("nativeOnResume");

    if (gWindow && !gRunning.load()) {
        startRenderThreadLocked();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnPause(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lk(gMutex);
    LOGI("nativeOnPause");
    stopRenderThreadLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeShutdown(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(gMutex);
    LOGI("nativeShutdown");

    stopRenderThreadLocked();
    releaseWindowLocked();

#if defined(FONLINE_ENGINE_PRESENT) && FONLINE_ENGINE_PRESENT
    if (gEngineReady.load()) {
        LOGI("Shutting down engine");
        AndroidClientShutdown();
        gEngineReady.store(false);
    }
#endif

    if (gAssetMgrObj) {
        env->DeleteGlobalRef(gAssetMgrObj);
        gAssetMgrObj = nullptr;
        gAssetMgr = nullptr;
    }
    gFilesDir.clear();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_fonline_NativeBridge_nativeOnTouch(JNIEnv*, jobject,
                                                   jint actionMasked, jint pointerId,
                                                   jfloat x, jfloat y, jboolean isPrimaryAction) {
    (void)pointerId;

    if (isPrimaryAction) {
        gLastX.store(x);
        gLastY.store(y);

        // ACTION_DOWN=0, ACTION_POINTER_DOWN=5
        if (actionMasked == 0 || actionMasked == 5) {
            gTouchCount.fetch_add(1);
        }
    }
}