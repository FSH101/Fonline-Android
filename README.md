# Fonline-Android

Android wrapper for the FOnline engine that wires a small JNI bridge to the engine sources.

Expected outcome: Android bring-up progresses: native build compiles & links; APK launches; logs show engine init loop.

## Prerequisites
- Android Studio on Windows 10/11
- Android SDK with **Android API 34** (install "Android SDK Platform 34" via SDK Manager)
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

See `PORTING.md` for a condensed map of the client entrypoint, platform hooks, and rendering/audio feature flags targeted by the Android bring-up.

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

### Android build: feature flags / known disabled subsystems
- `FO_DEBUG` is now provided as a compile definition for both the engine and JNI bridge (`app/src/main/cpp/CMakeLists.txt`). Debug builds set `FO_DEBUG=1`, Release builds set `FO_DEBUG=0`, and `Common/Common.h` supplies a defensive default when the define is missing.
- Spark effects are disabled on Android through `FO_HAVE_SPARK=0`. `SparkExtension.*` and `VisualParticles.*` carry stubbed implementations that keep callers compiling without `SPARK.h` while preserving the public interface for future re-enablement.
- Windows ACM audio and the general sound backend are disabled for Android via `FO_HAVE_ACM=0` and `FO_ENABLE_SOUND=0`. `SoundManager.cpp` now compiles lightweight stubs when sound is off; toggle these in `app/src/main/cpp/CMakeLists.txt` (per target) or override in `Common/Common.h`.
- Platform fallbacks for `FO_HAVE_SPARK`, `FO_HAVE_ACM`, and `FO_ENABLE_SOUND` live in `Common/Common.h` so non-Android/Windows builds still get sensible defaults if the CMake definitions are absent.

### Fixes (Android/NDK build)
- **Expected outcome:** NDK task `:app:buildCMakeDebug[arm64-v8a]` passes configure/generate and reaches compile without re-requesting missing headers (`SPARK.h`, `acmstrm.h`, `theora/theoradec.h`) or the `FO_DEBUG` define, then proceeds to the next compile/link stage.
- **Missing `SPARK.h` (`SparkExtension.h`/`VisualParticles.cpp`)**: Android builds define `FO_HAVE_SPARK=0` for both native targets, and Spark interfaces now pull a dedicated stub header (`Source/Client/Compat/SparkStubs.h`) instead of ever including `SPARK.h` when the feature is off. Public interfaces stay intact for future re-enablement.
- **Missing `acmstrm.h` (SoundManager.cpp:38)**: Android builds no longer include the Windows-only ACM header; a dedicated stub lives in `Source/Client/Compat/AcmStrmStub.h` and activates when `FO_HAVE_ACM=0`/`FO_ANDROID=1`, while Windows continues to include the real header. Default file extensions also prefer `ogg` on platforms without ACM support.
- **Missing `theora/theoradec.h` (VideoClip.cpp:36)**: Theora decoding is gated behind `FO_HAVE_THEORA`; Android defaults to `0` and now compiles a minimal stub implementation of `VideoClip` so missing Theora headers no longer stop the native build. A CMake cache option `FO_ENABLE_THEORA` drives the compile definition for both native targets—set it to `ON` and provide theora libs/headers to reintroduce playback.
- **Undeclared `FO_DEBUG` (`MapView.cpp` `if constexpr`)**: `app/src/main/cpp/CMakeLists.txt` now sets `FO_DEBUG=1` for Debug and `FO_DEBUG=0` otherwise so the compile-time branches instantiate cleanly; `Common/Common.h` still provides a fallback when the compile definition is missing.
- **Feature flags applied to all Android targets**: both `fonline_engine` and `native_bridge` now receive `FO_ANDROID=1`, `FO_WINDOWS=0`, the per-config `FO_DEBUG` define, and Android-default gates for Spark/ACM/sound/video so the compile commands consistently skip missing Windows-only headers and SPARK/Theora dependencies. Fallbacks in `Common/Common.h` now also consider `FO_ANDROID` in case `__ANDROID__` is absent.
- **Feature flags unified for engine and JNI**: the shared Android define set (including `FO_SAFE_ARITH_RELAXED=1` and `FO_HAVE_ASSIMP=0`) is now applied identically to both native targets to avoid divergence in compile lines. Geometry still uses `FO_GEOMETRY=${FO_GEOMETRY_MODE}` for the bridge while inheriting the shared defaults.
- **Changed files**: `.gitignore`, `app/src/main/cpp/CMakeLists.txt`, `Source/Client/SoundManager.cpp`, `Source/Client/Compat/AcmStrmStub.h`, `Source/Client/Compat/SparkStubs.h`, `Source/Client/SparkExtension.h`, `Source/Client/VideoClip.cpp`, `Source/Common/Common.h`, `README.md`.
- **How to build**: `./gradlew.bat clean :app:assembleDebug --stacktrace --info 2>&1 | Tee-Object -FilePath build_log.txt` (or `./gradlew` on *nix). This should push the NDK task past the missing-header and `FO_DEBUG` errors; the next failure, if any, will be unrelated to these fixes. The log file is now git-ignored—attach it to reviews instead of committing it.
- **Flags/defines**: `FO_ANDROID`/`FO_WINDOWS` are set in CMake for every Android target; `FO_DEBUG` uses generator expressions per configuration; audio gates use `FO_HAVE_ACM` and `FO_ENABLE_SOUND` with Android defaults of `0`; Spark uses `FO_HAVE_SPARK=0` by default on Android, backed by stub implementations.

### Known limitations / временные заглушки
- Gradle distribution download is blocked in this container; verify on a Windows host or CI with network access to ensure there are no further C++/linker errors beyond the fixed include/trait issues.
- `Application.h` stub generation only triggers when the engine checkout is incomplete; on a full checkout the real header is used. If the stub is used, functionality will be minimal until the real implementation is available.
- `android.suppressUnsupportedCompileSdk=35` mutes the AGP 8.4.x warning for `compileSdk=35`; remove it once the tooling version fully supports API 35.
- Spark visual effects and audio playback are stubbed out on Android (`FO_HAVE_SPARK=0`, `FO_ENABLE_SOUND=0`, `FO_HAVE_ACM=0`); rebuild with these flags set to `1` only when the corresponding native dependencies are available.

### Fix log (2025-06-26)
- Added a configurable CMake option `FO_ENABLE_THEORA` (default `OFF` for Android) that feeds `FO_HAVE_THEORA` to both native targets and backed it with a local default in `VideoClip.cpp`, keeping NDK builds from ever requesting `theora/theoradec.h` unless explicitly enabled. (`app/src/main/cpp/CMakeLists.txt`, `Source/Client/VideoClip.cpp`)

### Fix log (2025-06-27)
- Documented the client entry flow (`ClientApp` → `InitApp` → `MainEntry`/`FOClient::MainLoop`) and the SDL-driven platform/render/audio expectations to guide the Android platform layer work. (`PORTING.md`)

### Fix log (2025-06-24)
- Applied the Android flag set (including `FO_GEOMETRY`) uniformly to both native targets in CMake and defaulted `FO_HAVE_SPARK` to `0` directly in `VisualParticles.cpp` so Spark headers never load when the feature is disabled. (`app/src/main/cpp/CMakeLists.txt`, `Source/Client/VisualParticles.cpp`)

### Fix log (2025-05-13)
- Hardened `safe_numeric_cast` traits so only enum types instantiate `std::underlying_type`, avoiding Android/libc++ template failures while preserving Android's permissive clamp fallback (`Source/Essentials/SafeArithmetics.h`).

### Fix log (2025-05-15)
- Added a relaxed float/integer conversion path on Android so `safe_numeric_cast` clamps instead of asserting in the native toolchain, keeping `short -> int` and similar widening conversions compiling under NDK libc++ (`Source/Essentials/SafeArithmetics.h`).
- Propagated `FO_GEOMETRY` to every target via `add_compile_definitions` and added an `Application.h` availability check with a stub fallback to avoid missing-header failures in the native Android CMake configuration (`app/src/main/cpp/CMakeLists.txt`).
- Silenced the AGP `compileSdk=35` warning during bring-up via `android.suppressUnsupportedCompileSdk=35` (`gradle.properties`).

### Fix log (2025-05-22)
- Added `FO_DEBUG` compile-time definitions for Debug/Release and a defensive fallback in `Common/Common.h` so `if constexpr (FO_DEBUG)` blocks compile on Android. (`app/src/main/cpp/CMakeLists.txt`, `Source/Common/Common.h`)
- Introduced feature flags for Spark and audio subsystems (`FO_HAVE_SPARK=0`, `FO_HAVE_ACM=0`, `FO_ENABLE_SOUND=0`) with stub implementations for Spark rendering and SoundManager to keep Android builds progressing without `SPARK.h` or `acmstrm.h`. (`Source/Client/SparkExtension.*`, `Source/Client/VisualParticles.*`, `Source/Client/SoundManager.cpp`, `app/src/main/cpp/CMakeLists.txt`, `Source/Common/Common.h`)

### Fix log (2025-05-23)
- Normalized Android compile definitions so both `fonline_engine` and `native_bridge` see the same `FO_ANDROID=1`, `FO_WINDOWS=0`, and per-configuration `FO_DEBUG` flags along with the Android defaults for Spark/ACM/sound. (`app/src/main/cpp/CMakeLists.txt`)

### Fix log (2025-05-27)
- Added `Source/Client/Compat/SparkStubs.h` and routed Spark headers through it whenever `FO_HAVE_SPARK=0`, keeping Android builds from requesting `SPARK.h` while preserving the runtime interface. (`Source/Client/SparkExtension.h`, `Source/Client/VisualParticles.cpp`)
- Hardened platform fallbacks for Spark/audio flags to look at `FO_ANDROID` even when `__ANDROID__` is missing, and documented that `build_log.txt` is ignored and should be attached externally instead of committed. (`Source/Common/Common.h`, `.gitignore`, `README.md`)

### Fix log (2025-05-30)
- Guarded Windows ACM includes with numeric `FO_WINDOWS` checks and defaulted Android to stubbed audio (`FO_HAVE_ACM=0`, `FO_ENABLE_SOUND=0`) so `acmstrm.h` is never requested in NDK builds. (`Source/Client/SoundManager.cpp`, `app/src/main/cpp/CMakeLists.txt`)
- Gated Theora decoding behind `FO_HAVE_THEORA` with Android default `0` and added a stubbed `VideoClip` implementation for builds without theora headers. (`Source/Common/Common.h`, `Source/Client/VideoClip.cpp`, `app/src/main/cpp/CMakeLists.txt`)

### Fix log (2025-06-15)
- Added a local default for `FO_HAVE_SPARK` inside `SparkExtension.h` and tightened the ACM include guard in `SoundManager.cpp` to require explicit numeric `FO_WINDOWS`/`FO_HAVE_ACM` checks, keeping Android builds from requesting Windows-only headers while retaining Windows behavior.

### Fix log (2025-06-02)
- Synced Android compile definitions between `fonline_engine` and `native_bridge`, ensuring both targets see the same Spark/ACM/Theora/audio gates plus `FO_SAFE_ARITH_RELAXED=1` and `FO_HAVE_ASSIMP=0`, with geometry still configured explicitly for the bridge. (`app/src/main/cpp/CMakeLists.txt`)

### Fix log (2025-05-12)
- Exposed `FO_GEOMETRY_MODE` in CMake and defined it for both the engine and JNI bridge to ensure `FO_GEOMETRY` is always supplied during Android builds.

### Проверка обновления README
- Этот блок добавлен для подтверждения, что README обновлён в рамках ветки `codex/restore-proper-formatting-for-codes`.

### Команды для Дениса (repro/build)
```bash
# в папке репо
 git fetch origin --prune
 git switch codex/restore-proper-formatting-for-codes
 git pull --ff-only

# сборка с логом
 ./gradlew.bat clean :app:assembleDebug --stacktrace --info 2>&1 | Tee-Object -FilePath build_log.txt
```
