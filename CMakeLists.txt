cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(AdaptiveMesh VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)            # Strictly disable compiler-specific extensions
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # Enable PIC for shared libraries/PIE

# ==============================================================================
# Warnings configuration
# ==============================================================================
option(SOAM_WARNINGS_AS_ERRORS "Treat warnings as errors (recommended for CI)" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(SOAM_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
    if(SOAM_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
endif()

# ==============================================================================
# Sanitizers (Debug builds only) via Generator Expressions
# ==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
    add_compile_options("$<$<CONFIG:Debug>:-fno-omit-frame-pointer>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
endif()

# ==============================================================================
# Targets & Includes
# ==============================================================================
add_library(adaptive_mesh INTERFACE)
target_include_directories(adaptive_mesh INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ==============================================================================
# Testing
# ==============================================================================
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(mesh_tests tests/mesh_tests.cpp)
    target_link_libraries(mesh_tests PRIVATE adaptive_mesh)
    add_test(NAME run_mesh_tests COMMAND mesh_tests)
    set_tests_properties(run_mesh_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )

    add_executable(api_access_tests tests/api_access_tests.cpp)
    target_link_libraries(api_access_tests PRIVATE adaptive_mesh)
    add_test(NAME run_api_access_tests COMMAND api_access_tests)
    set_tests_properties(run_api_access_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()

# ==============================================================================
# Benchmarking
# ==============================================================================
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_executable(mesh_benchmark benchmarks/mesh_benchmark.cpp)
    target_link_libraries(mesh_benchmark PRIVATE adaptive_mesh)
endif()
