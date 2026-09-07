cmake_minimum_required(VERSION 3.20)

project(
    soam_k9_kiko_interchange
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

get_filename_component(
    SOAM_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.."
    ABSOLUTE)

add_library(
    soam_k9_interchange
    STATIC
    k9_interchange.cpp)

target_include_directories(
    soam_k9_interchange
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}")

if(MSVC)
    target_compile_options(
        soam_k9_interchange
        PRIVATE
        /W4)
else()
    target_compile_options(
        soam_k9_interchange
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic)
endif()

add_executable(
    soam_k9_interchange_tests
    k9_interchange_tests.cpp)

target_link_libraries(
    soam_k9_interchange_tests
    PRIVATE
    soam_k9_interchange)

if(MSVC)
    target_compile_options(
        soam_k9_interchange_tests
        PRIVATE
        /W4)
else()
    target_compile_options(
        soam_k9_interchange_tests
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic)
endif()

add_test(
    NAME soam_k9_interchange_tests
    COMMAND soam_k9_interchange_tests)

add_executable(
    soam_k9_isolation_tests
    k9_isolation_tests.cpp)

target_link_libraries(
    soam_k9_isolation_tests
    PRIVATE
    soam_k9_interchange)

target_include_directories(
    soam_k9_isolation_tests
    PRIVATE
    "${SOAM_ROOT}/include")

if(MSVC)
    target_compile_options(
        soam_k9_isolation_tests
        PRIVATE
        /W4)
else()
    target_compile_options(
        soam_k9_isolation_tests
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic)
endif()

add_test(
    NAME soam_k9_isolation_tests
    COMMAND soam_k9_isolation_tests)
