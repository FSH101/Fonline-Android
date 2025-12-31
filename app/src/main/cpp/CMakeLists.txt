cmake_minimum_required(VERSION 3.22.1)

project(fonline_android LANGUAGES C CXX)

# FOnline master requires at least C++20 (BasicCore.h checks __cplusplus)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Our JNI / ANativeWindow bridge (already works).
add_library(native_bridge SHARED
        native_bridge.cpp)

# Android system libs
find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(glesv2-lib GLESv2)

target_link_libraries(native_bridge
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${glesv2-lib})

# ============================================================
# FOnline engine integration (first compile attempt)
# ============================================================
# Expected layout:
# <repo_root>/engine_src/fonline-master/Source/...
set(PROJ_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(ENGINE_DIR "${PROJ_ROOT}/engine_src/fonline-master")

option(FONLINE_WITH_ENGINE "Try to build FOnline engine sources" ON)

if(FONLINE_WITH_ENGINE AND EXISTS "${ENGINE_DIR}/Source")
    message(STATUS "FOnline engine found at: ${ENGINE_DIR}")

    file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
            "${ENGINE_DIR}/Source/*.c"
            "${ENGINE_DIR}/Source/*.cc"
            "${ENGINE_DIR}/Source/*.cpp"
            "${ENGINE_DIR}/Source/*.cxx")

    # Drop things we definitely do NOT want in an Android lib build.
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Applications/")
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Tests?/")         # Test / Tests
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/Source/Tools?/")         # Tool / Tools
    list(FILTER ENGINE_SRC EXCLUDE REGEX "/ThirdParty/")            # If Source accidentally includes it
    list(FILTER ENGINE_SRC EXCLUDE REGEX "Rendering-Direct3D\.cpp$")
    list(FILTER ENGINE_SRC EXCLUDE REGEX "WinApi-.*\.cpp$")

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
    target_include_directories(fonline_engine PUBLIC
            "${ENGINE_DIR}/Source"
            "${ENGINE_DIR}/Source/Common"
            "${ENGINE_DIR}/Source/Client"
            "${ENGINE_DIR}/Source/Server"
            # ankerl::unordered_dense is now stored under ThirdParty/ankerl
            "${ENGINE_DIR}/ThirdParty"
            "${ENGINE_DIR}/ThirdParty/ankerl/include")

    # These defs are guesses — they are here to help you quickly see
    # what breaks first. We'll adjust after the first compiler error batch.
    target_compile_definitions(fonline_engine PUBLIC
            __ANDROID__=1
            ANDROID=1
            FO_ANDROID=1
            FO_WINDOWS=0)

    # Many engines assume exceptions/RTTI in the core; keep them on for now.
    target_compile_options(fonline_engine PRIVATE -fexceptions -frtti)

    target_link_libraries(fonline_engine
            ${log-lib}
            ${android-lib}
            ${egl-lib}
            ${glesv2-lib})

    # Link engine into our JNI bridge so it ends up in the .apk.
    target_link_libraries(native_bridge fonline_engine)

    target_compile_definitions(native_bridge PRIVATE FONLINE_ENGINE_PRESENT=1)
else()
    message(STATUS "FOnline engine not built (missing sources or FONLINE_WITH_ENGINE=OFF).")
endif()
