cmake_minimum_required(VERSION 3.22.1)
project(native_bridge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(native-bridge SHARED
        native_bridge.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(native-bridge
        ${log-lib}
        ${android-lib}
)
