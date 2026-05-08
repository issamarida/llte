cmake_minimum_required(VERSION 3.25)
project(llte
    VERSION 0.1.0
    DESCRIPTION "Low-latency multi-exchange trading engine"
    LANGUAGES CXX
)

# Default to Release if not specified. We almost always want optimised builds
# for benchmarks; Debug builds are only useful for stepping through code.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)         # use -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # required for clangd / IDE integration

include(cmake/CompilerWarnings.cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- Core library (header-only at this stage) -------------------------------
add_library(llte INTERFACE)
target_include_directories(llte INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(llte INTERFACE cxx_std_20)
llte_set_warnings(llte INTERFACE)

# --- Dependencies via FetchContent -----------------------------------------
# FetchContent downloads + builds deps as part of our build. Simple and
# self-contained; we'll graduate to vcpkg/Conan when this becomes painful.
include(FetchContent)

# Catch2 — unit testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# Google Benchmark — microbenchmarks
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(benchmark)

# --- Subdirectories ---------------------------------------------------------
enable_testing()
add_subdirectory(tests)
add_subdirectory(benches)

message(STATUS "")
message(STATUS "  llte ${PROJECT_VERSION}")
message(STATUS "  build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "")
