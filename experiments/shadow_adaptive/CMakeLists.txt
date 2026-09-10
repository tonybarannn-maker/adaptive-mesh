cmake_minimum_required(VERSION 3.20)

project(soam_k10_shadow_adaptive LANGUAGES CXX)

find_package(AdaptiveMesh 2.0 CONFIG REQUIRED)

enable_testing()

add_library(
    k10_shadow_adaptive
    STATIC
    shadow_adaptive_controller.cpp
)

target_compile_features(
    k10_shadow_adaptive
    PUBLIC
    cxx_std_20
)

target_include_directories(
    k10_shadow_adaptive
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    k10_shadow_adaptive
    PUBLIC
    AdaptiveMesh::adaptive_mesh_domain
)

add_executable(
    k10_shadow_adaptive_tests
    shadow_adaptive_tests.cpp
)

target_link_libraries(
    k10_shadow_adaptive_tests
    PRIVATE
    k10_shadow_adaptive
)

add_test(
    NAME k10_shadow_adaptive_tests
    COMMAND k10_shadow_adaptive_tests
)

add_executable(
    k10_shadow_adaptive_scenarios
    shadow_adaptive_scenarios.cpp
)

target_link_libraries(
    k10_shadow_adaptive_scenarios
    PRIVATE
    k10_shadow_adaptive
)

add_test(
    NAME k10_shadow_adaptive_scenarios
    COMMAND k10_shadow_adaptive_scenarios
)
