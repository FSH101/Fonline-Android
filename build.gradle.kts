// Root-level build.gradle.kts (Project)
// ⚠️ If you already have versions that match your Android Studio, you can keep them.
// The important part of THIS task is setting Java/Kotlin target to 17 in app/build.gradle.kts.

plugins {
    // Use the versions that Android Studio предложит для твоей установки.
    // Ниже — пример из официальной доки AGP 8.13.x.
    id("com.android.application") version "8.13.2" apply false
    id("com.android.library") version "8.13.2" apply false
    id("org.jetbrains.kotlin.android") version "2.2.21" apply false
}
