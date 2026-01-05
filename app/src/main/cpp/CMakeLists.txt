cmake_minimum_required(VERSION 3.22.1)

project(fonline_android LANGUAGES C CXX)

# FOnline master requires at least C++20 (BasicCore.h checks __cplusplus)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build SDL3 from the bundled third-party sources for Android
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# SDL-driven entrypoint + JNI helpers.
add_library(main SHARED
        native_bridge.cpp
        sdl_main.cpp)

# Android system libs
find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv2-lib GLESv2)

target_link_libraries(main
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${glesv2-lib}
        SDL3::SDL3-static)

# ============================================================
# FOnline engine integration (first compile attempt)
# ============================================================
# Expected layout:
# <repo_root>/engine_src/fonline-master/Source/...
set(PROJ_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(ENGINE_DIR "${PROJ_ROOT}/engine_src/fonline-master")

add_subdirectory("${ENGINE_DIR}/ThirdParty/SDL" "${CMAKE_CURRENT_BINARY_DIR}/sdl-build")

option(FONLINE_WITH_ENGINE "Try to build FOnline engine sources" ON)
option(FO_ENABLE_SCRIPTING "Enable AngelScript scripting" OFF)

set(_DEFAULT_BUILD_SERVER ON)
set(_DEFAULT_BUILD_EDITOR ON)
if(ANDROID)
    set(_DEFAULT_BUILD_SERVER OFF)
    set(_DEFAULT_BUILD_EDITOR OFF)
endif()

option(FO_BUILD_SERVER "Build server-side applications" ${_DEFAULT_BUILD_SERVER})
option(FO_BUILD_EDITOR "Build editor applications" ${_DEFAULT_BUILD_EDITOR})

if(FONLINE_WITH_ENGINE AND EXISTS "${ENGINE_DIR}/Source")
    message(STATUS "FOnline engine found at: ${ENGINE_DIR}")

    file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
            "${ENGINE_DIR}/Source/*.c"
            "${ENGINE_DIR}/Source/*.cc"
            "${ENGINE_DIR}/Source/*.cpp"
            "${ENGINE_DIR}/Source/*.cxx")

    # Drop things we definitely do NOT want in an Android lib build.
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Tests?/")         # Test / Tests
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Tools?/")         # Tool / Tools
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/ThirdParty/")            # If Source accidentally includes it
    list(FILTER ENGINE_SRC EXCLUDE REGEX "Rendering-Direct3D\\.cpp$")
    list(FILTER ENGINE_SRC EXCLUDE REGEX "WinApi-.*\\.cpp$")

    if(NOT FO_BUILD_SERVER)
        # Android bring-up: server binaries and DB dependencies are not built
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Applications/Server.*\\.cpp$")
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Server(/|\\\\)")
    endif()

    if(NOT FO_BUILD_EDITOR)
        # Android bring-up: editor/UI layers depend on ImGui/Editor headers not present here
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Applications/Editor.*\\.cpp$")
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Editor(/|\\\\)")
    endif()

    if(ANDROID)
        # Android bring-up: ImGui/libbson not available, skip desktop/front-end and server DB entrypoints
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Frontend/Application\\.cpp$")
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Server/DataBase\\.cpp$")
        # Android bring-up: server is not built on this platform
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Server(/|\\\\)")
    endif()

    if(NOT FO_ENABLE_SCRIPTING)
        list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Scripting/")
    endif()

    add_library(fonline_engine STATIC ${ENGINE_SRC})

    # Enforce C++20 specifically on this target too (avoids -std=c++17 being appended later)
    target_compile_features(fonline_engine PUBLIC cxx_std_20)

    # -------------------------------------------------------------------------
    # Auto-fix include paths for "BasicCore.h" and its sibling headers like
    # "small_vector.hpp" (included from BasicCore.h).
    # -------------------------------------------------------------------------

    # BasicCore.h
    file(GLOB_RECURSE _BASICCORE_CANDIDATES
        "${ENGINE_DIR}/Source/**/BasicCore.h"
        "${ENGINE_DIR}/ThirdParty/**/BasicCore.h"
    )

    set(_BASICCORE_DIR "")

    # Prefer Android-specific header if present
    foreach(_cand IN LISTS _BASICCORE_CANDIDATES)
        if(ANDROID AND _cand MATCHES "/(Android|android)/")
            get_filename_component(_BASICCORE_DIR "${_cand}" DIRECTORY)
            break()
        endif()
    endforeach()

    # Otherwise prefer Unix/Posix/Linux variant
    if(NOT _BASICCORE_DIR)
        foreach(_cand IN LISTS _BASICCORE_CANDIDATES)
            if(_cand MATCHES "/(Unix|unix|Posix|posix|Linux|linux)/")
                get_filename_component(_BASICCORE_DIR "${_cand}" DIRECTORY)
                break()
            endif()
        endforeach()
    endif()

    # Fallback: first match
    if(NOT _BASICCORE_DIR AND _BASICCORE_CANDIDATES)
        list(GET _BASICCORE_CANDIDATES 0 _cand0)
        get_filename_component(_BASICCORE_DIR "${_cand0}" DIRECTORY)
    endif()

    if(_BASICCORE_DIR)
        message(STATUS "Using BasicCore include dir: ${_BASICCORE_DIR}")
        target_include_directories(fonline_engine PRIVATE "${_BASICCORE_DIR}")
    else()
        message(FATAL_ERROR "BasicCore.h not found under ${ENGINE_DIR}. Check your engine sources/submodules.")
    endif()

    # small_vector.hpp
    file(GLOB_RECURSE _SMALLVEC_CANDIDATES
        "${ENGINE_DIR}/Source/**/small_vector.hpp"
        "${ENGINE_DIR}/ThirdParty/**/small_vector.hpp"
    )

    set(_SMALLVEC_DIR "")

    # Prefer one living near Essentials (most common in FOnline layouts)
    foreach(_cand IN LISTS _SMALLVEC_CANDIDATES)
        if(_cand MATCHES "/Essentials/")
            get_filename_component(_SMALLVEC_DIR "${_cand}" DIRECTORY)
            break()
        endif()
    endforeach()

    # Fallback: first match
    if(NOT _SMALLVEC_DIR AND _SMALLVEC_CANDIDATES)
        list(GET _SMALLVEC_CANDIDATES 0 _sv0)
        get_filename_component(_SMALLVEC_DIR "${_sv0}" DIRECTORY)
    endif()

    if(_SMALLVEC_DIR)
        message(STATUS "Using small_vector include dir: ${_SMALLVEC_DIR}")
        target_include_directories(fonline_engine PRIVATE "${_SMALLVEC_DIR}")
    else()
        message(FATAL_ERROR "small_vector.hpp not found under ${ENGINE_DIR}. Your engine checkout is incomplete.")
    endif()

    # IMPORTANT:
    # Many headers use bare includes like: #include "Common.h"
    # In FOnline sources, Common.h lives in Source/Common/Common.h,
    # so we MUST add Source/Common to include dirs (and a few other common roots).
    if(ANDROID)
        target_include_directories(fonline_engine PUBLIC
                "${ENGINE_DIR}/Source"
                "${ENGINE_DIR}/Source/Common"
                "${ENGINE_DIR}/Source/Client"
                "${ENGINE_DIR}/Source/Frontend"
                "${ENGINE_DIR}/Source/Frontend/Applications"
                "${ENGINE_DIR}/Source/Generated"
                # ankerl::unordered_dense is now stored under ThirdParty/ankerl
                "${ENGINE_DIR}/ThirdParty"
                "${ENGINE_DIR}/ThirdParty/ankerl/include"
                # header-only fmt fallback for Android builds
                "${ENGINE_DIR}/ThirdParty/fmt/include")

        target_include_directories(main PRIVATE
                "${ENGINE_DIR}/Source"
                "${ENGINE_DIR}/Source/Common"
                "${ENGINE_DIR}/Source/Frontend"
                "${ENGINE_DIR}/Source/Frontend/Applications"
                "${ENGINE_DIR}/Source/Generated")
    else()
        target_include_directories(fonline_engine PUBLIC
                "${ENGINE_DIR}/Source"
                "${ENGINE_DIR}/Source/Common"
                "${ENGINE_DIR}/Source/Client"
                "${ENGINE_DIR}/Source/Server"
                "${ENGINE_DIR}/Source/Frontend"
                "${ENGINE_DIR}/Source/Frontend/Applications"
                "${ENGINE_DIR}/Source/Generated"
                # ankerl::unordered_dense is now stored under ThirdParty/ankerl
                "${ENGINE_DIR}/ThirdParty"
                "${ENGINE_DIR}/ThirdParty/ankerl/include"
                # header-only fmt fallback for Android builds
                "${ENGINE_DIR}/ThirdParty/fmt/include")

        target_include_directories(main PRIVATE
                "${ENGINE_DIR}/Source"
                "${ENGINE_DIR}/Source/Common"
                "${ENGINE_DIR}/Source/Frontend"
                "${ENGINE_DIR}/Source/Frontend/Applications"
                "${ENGINE_DIR}/Source/Generated")
    endif()

    # These defs are guesses — they are here to help you quickly see
    # what breaks first. We'll adjust after the first compiler error batch.
    set(FO_GEOMETRY_MODE 2 CACHE STRING "FOnline geometry mode (1=hex, 2=square)")
    option(FO_ENABLE_THEORA "Enable Theora video decoding if headers/libs are available" OFF)

    set(_APP_HEADER "${ENGINE_DIR}/Source/Frontend/Application.h")
    if(EXISTS "${_APP_HEADER}")
        message(STATUS "Found Application.h at: ${_APP_HEADER}")
    else()
        set(_APP_STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_stubs")
        file(MAKE_DIRECTORY "${_APP_STUB_DIR}")
        set(_APP_STUB "${_APP_STUB_DIR}/Application.h")
        if(NOT EXISTS "${_APP_STUB}")
            file(WRITE "${_APP_STUB}"
"#pragma once\n"
"// Minimal stub used only when the engine's Application.h is absent during Android bring-up.\n"
"struct SDL_Window;\n"
"namespace FO_NAMESPACE { struct Window; class Application {}; }\n"
)
        endif()
        message(WARNING "Application.h missing under engine sources; using generated stub at ${_APP_STUB}")
        target_include_directories(fonline_engine PUBLIC "${_APP_STUB_DIR}")
        target_include_directories(main PRIVATE "${_APP_STUB_DIR}")
    endif()

    set(ANDROID_ENGINE_COMMON_DEFS
            $<$<CONFIG:Debug>:FO_DEBUG=1>
            $<$<NOT:$<CONFIG:Debug>>:FO_DEBUG=0>
            __ANDROID__=1
            ANDROID=1
            FO_ANDROID=1
            FO_WINDOWS=0
            FO_SAFE_ARITH_RELAXED=1
            FO_HAVE_ASSIMP=0
            FO_HAVE_SPARK=0
            FO_HAVE_ACM=0
            FO_HAVE_THEORA=0
            FO_ENABLE_SOUND=0
            FO_ENABLE_SCRIPTING=$<BOOL:${FO_ENABLE_SCRIPTING}>
            FO_GEOMETRY=${FO_GEOMETRY_MODE}
            SDL_MAIN_HANDLED=1)

    target_compile_definitions(fonline_engine PUBLIC ${ANDROID_ENGINE_COMMON_DEFS})

    target_compile_definitions(main PRIVATE ${ANDROID_ENGINE_COMMON_DEFS})

    # Many engines assume exceptions/RTTI in the core; keep them on for now.
    target_compile_options(fonline_engine PRIVATE -fexceptions -frtti -ftemplate-backtrace-limit=0 -ferror-limit=5)

    target_link_libraries(fonline_engine
            ${log-lib}
            ${android-lib}
            ${egl-lib}
            ${glesv2-lib}
            SDL3::SDL3-static)

    # Link engine into our JNI bridge so it ends up in the .apk.
    target_link_libraries(main fonline_engine)

    target_compile_definitions(main PRIVATE FONLINE_ENGINE_PRESENT=1)
else()
    message(STATUS "FOnline engine not built (missing sources or FONLINE_WITH_ENGINE=OFF).")
endif()
