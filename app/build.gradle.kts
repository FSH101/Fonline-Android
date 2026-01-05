// Module-level app/build.gradle.kts
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.example.fonline"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.example.fonline"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"

        // NDK / ABI
        ndk {
            abiFilters.addAll(listOf("armeabi-v7a", "arm64-v8a", "x86", "x86_64"))
        }

        // Здесь ТОЛЬКО флаги/аргументы CMake, НЕ path.
        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf("-std=c++20", "-frtti", "-fexceptions"))
                arguments.addAll(listOf("-DANDROID_STL=c++_shared"))
            }
        }
    }

    // ✅ ФИКС "Unknown Kotlin JVM target: 21"
    // Android/AGP по умолчанию любит JDK 17.
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }

    // Пинить версии можно, но если у тебя стоят другие — поменяй под свои.
    ndkVersion = "26.3.11579264"

    // ✅ Путь к CMakeLists.txt задаётся ТОЛЬКО здесь.
    externalNativeBuild {
        cmake {
            // Вариант A (часто работает):
            path = file("src/main/cpp/CMakeLists.txt")
            // Вариант B (если попросит String):
            // path = file("src/main/cpp/CMakeLists.txt").path

            version = "3.22.1"
        }
    }

    buildTypes {
        debug {
            isJniDebuggable = true
            ndk {
                abiFilters.clear()
                abiFilters.add("arm64-v8a")
            }
        }
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }

    buildFeatures {
        viewBinding = true
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
        resources {
            excludes += setOf("META-INF/*")
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
}
