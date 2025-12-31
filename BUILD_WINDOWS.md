# Windows build notes

## Prerequisites
- Install Android Studio (includes JetBrains Runtime).
- Install SDK/NDK components via the Android Studio SDK Manager:
  - Android SDK Platform and Platform Tools
  - CMake
  - NDK (r26 or newer is recommended)
- Add the SDK `platform-tools` directory to your `PATH` so Gradle can find `adb` when needed.

## Building from the command line
Run the assembly from the repository root and capture output:

```
./gradlew.bat clean :app:assembleDebug --stacktrace --info > build-windows.log 2>&1
```

If Gradle fails to download because of network restrictions, see the proxy section in `README.md` for configuration options.
