# Fonline-Android

Android wrapper for the FOnline engine that wires a small JNI bridge to the engine sources.

## Prerequisites
- Android Studio on Windows 10/11
- Android SDK with **Android API 35**
- **NDK 26.3.11579264** (matching the version pinned in `app/build.gradle.kts`)
- CMake **3.22.1** (installed via Android Studio's SDK Manager)

## Building
```bash
# From the repository root
./gradlew :app:assembleDebug
```
On Windows use `gradlew.bat :app:assembleDebug` instead.

The Gradle scripts already configure the external native build to use `app/src/main/cpp/CMakeLists.txt`. Engine sources are resolved under `engine_src/fonline-master` automatically; no manual path edits are required.

## Native sources layout
- `app/src/main/cpp` — JNI bridge (`native_bridge.cpp`) and CMake entry point.
- `engine_src/fonline-master` — FOnline engine checkout.
- Third-party headers (e.g., `ankerl/unordered_dense.h`) live under `engine_src/fonline-master/ThirdParty` and are included via CMake include paths.

## Notes
- `local.properties` is intentionally git-ignored; Android Studio will regenerate it with your local SDK/NDK paths.
- If you enable additional ABIs or change toolchain versions, keep them in sync with the values in `app/build.gradle.kts` to avoid mismatch errors.
