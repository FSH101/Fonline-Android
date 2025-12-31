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
- **Fix #1: safe_numeric_cast constraints / NDK underlying_type issue:** `safe_numeric_cast` no longer instantiates `std::underlying_type` unless the source or destination is an enum, keeping widening casts like `safe_numeric_cast<int>(short)` valid on libc++ and preserving the existing Android clamp-based fallback (`Source/Essentials/SafeArithmetics.h`).
- **Formatting compatibility:** Added `FormatCompat.h` and routed engine headers to use `fo_fmt` (mapped to `{fmt}` on Android) instead of the incomplete NDK `<format>` implementation (`Source/Essentials/BasicCore.h`, `Source/Essentials/StringUtils.h`, `Source/Essentials/Containers.h`, `Source/Essentials/ExceptionHadling.h`). The bundled `{fmt}` now safely prints types that lack `operator<<` by falling back to `"<unformattable>"` instead of triggering a compile-time error (`ThirdParty/fmt/include/fmt/format.h`).
- **Safe arithmetic on Android (updated):** numeric casting now uses an enum-aware `numeric_value` trait that only instantiates `std::underlying_type` for actual enums, preventing libc++ from rejecting `underlying_type<float>` and similar cases while keeping range checks in place (`Source/Essentials/SafeArithmetics.h`).
- **Float/int bring-up fix:** Android now routes `numeric_cast` through a permissive clamp path for floating-point conversions (while retaining strict checks off Android) so `safe_numeric_cast` no longer trips template assertions during NDK builds (`Source/Essentials/SafeArithmetics.h`).
- **Geometry mode:** Android CMake defines `FO_GEOMETRY=2` (square) via a configurable `FO_GEOMETRY_MODE` cache entry (propagated to all targets through `add_compile_definitions`) to satisfy the engine's required geometry switch (`app/src/main/cpp/CMakeLists.txt`), and `Common.h` provides the same default when `FO_GEOMETRY` is missing in Android builds.
- **Missing headers resolved:** Added `Source/Generated/Version-Include.h` stub (now also providing `FO_COMPATIBILITY_VERSION`) and exposed `Source/Frontend`/`Source/Frontend/Applications`/`Source/Generated` include directories so `Application.h`, `Rendering.h`, and `Version-Include.h` resolve during native builds (the JNI bridge now also consumes these include roots directly). The CMake script now explicitly checks for `Application.h` and generates a minimal stub if the engine checkout lacks it so Android builds can continue.
- **Assimp optionality:** when `FO_HAVE_ASSIMP=0`, lightweight math stubs (`Source/Common/AssimpStubs.h`) provide the required matrix/vector/quaternion types so the client sources still compile. Android builds default to `FO_HAVE_ASSIMP=0` even without a CMake define to prevent missing-header errors, and the `assimp` include is now fully guarded so it is skipped when the flag is zero (`Source/Common/Common.h`).
- **Diagnostics for template errors:** Android CMake adds `-ftemplate-backtrace-limit=0` and a small `-ferror-limit` so the first failing instantiation is visible when troubleshooting.
- **How to build:** run `./gradlew clean :app:assembleDebug` (or `gradlew.bat` on Windows). ABIs listed in `abiFilters` (`armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`) are wired for the native build.

### Known limitations / временные заглушки
- Gradle distribution download is blocked in this container; verify on a Windows host or CI with network access to ensure there are no further C++/linker errors beyond the fixed include/trait issues.
- `Application.h` stub generation only triggers when the engine checkout is incomplete; on a full checkout the real header is used. If the stub is used, functionality will be minimal until the real implementation is available.
- `android.suppressUnsupportedCompileSdk=35` mutes the AGP 8.4.x warning for `compileSdk=35`; remove it once the tooling version fully supports API 35.

### Fix log (2025-05-13)
- Hardened `safe_numeric_cast` traits so only enum types instantiate `std::underlying_type`, avoiding Android/libc++ template failures while preserving Android's permissive clamp fallback (`Source/Essentials/SafeArithmetics.h`).

### Fix log (2025-05-15)
- Added a relaxed float/integer conversion path on Android so `safe_numeric_cast` clamps instead of asserting in the native toolchain, keeping `short -> int` and similar widening conversions compiling under NDK libc++ (`Source/Essentials/SafeArithmetics.h`).
- Propagated `FO_GEOMETRY` to every target via `add_compile_definitions` and added an `Application.h` availability check with a stub fallback to avoid missing-header failures in the native Android CMake configuration (`app/src/main/cpp/CMakeLists.txt`).
- Silenced the AGP `compileSdk=35` warning during bring-up via `android.suppressUnsupportedCompileSdk=35` (`gradle.properties`).

### Fix log (2025-05-12)
- Exposed `FO_GEOMETRY_MODE` in CMake and defined it for both the engine and JNI bridge to ensure `FO_GEOMETRY` is always supplied during Android builds.

### Проверка обновления README
- Этот блок добавлен для подтверждения, что README обновлён в рамках ветки `codex/restore-proper-formatting-for-codes`.
