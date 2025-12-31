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

## Если Gradle не скачивается из-за прокси/блокировок
1. **Настроить `~/.gradle/gradle.properties`.**
   - Создайте файл, либо скопируйте `gradle.properties.example` в домашний каталог и пропишите `systemProp.http.proxyHost`, `systemProp.http.proxyPort`, `systemProp.https.proxyHost`, `systemProp.https.proxyPort`. При необходимости добавьте `systemProp.*.proxyUser`/`proxyPassword` и `systemProp.http.nonProxyHosts`.
2. **Использовать переменные окружения.**
   - Перед запуском `./gradlew` выставьте `HTTP_PROXY`/`HTTPS_PROXY` (или нижним регистром) в виде `http://user:pass@host:port`.
3. **Подложить дистрибутив вручную.**
   - Скачайте ZIP для версии Gradle, указанной в `gradle/wrapper/gradle-wrapper.properties` (`gradle-8.6-bin.zip`), проверьте SHA-256 с [официальной страницы](https://gradle.org/releases/), затем положите архив в кеш `~/.gradle/wrapper/dists/gradle-8.6-bin/` (папку с именем-хешем создаёт сам Gradle). После этого `./gradlew` возьмёт локальный архив без выхода в интернет.

## Native sources layout
- `app/src/main/cpp` — JNI bridge (`native_bridge.cpp`) and CMake entry point.
- `engine_src/fonline-master` — FOnline engine checkout.
- Third-party headers (e.g., `ankerl/unordered_dense.h`) live under `engine_src/fonline-master/ThirdParty` and are included via CMake include paths.

## Notes
- `local.properties` is intentionally git-ignored; Android Studio will regenerate it with your local SDK/NDK paths.
- If you enable additional ABIs or change toolchain versions, keep them in sync with the values in `app/build.gradle.kts` to avoid mismatch errors.

## Fixes (codex/restore-proper-formatting-for-codes)
- Safe arithmetic templates now normalize enum/unsigned/signed conversions instead of tripping static assertions during Android builds.
- Assimp headers are optional on Android: when `FO_HAVE_ASSIMP=0`, lightweight math stubs (`AssimpStubs.h`) provide the required matrix/vector/quaternion types so the client sources still compile.
- Android CMake adds clearer template diagnostics (`-ftemplate-backtrace-limit=0`, `-ferror-limit=5`) to surface the first failing instantiation when troubleshooting.
- Build with `./gradlew clean :app:assembleDebug` (or `gradlew.bat` on Windows); ABIs listed in `abiFilters` (`armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`) are wired for the native build.
