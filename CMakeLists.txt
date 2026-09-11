cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(AdaptiveMesh VERSION 2.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SOAM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(BUILD_EXAMPLES "Build examples" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  if(SOAM_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
elseif(MSVC)
  add_compile_options(/W4)
  if(SOAM_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
endif()

add_library(adaptive_mesh_domain INTERFACE)
add_library(AdaptiveMesh::adaptive_mesh_domain ALIAS adaptive_mesh_domain)
target_compile_features(adaptive_mesh_domain INTERFACE cxx_std_20)
target_include_directories(adaptive_mesh_domain INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

function(soam_configure_runtime_header output_dir profile scenario)
  set(SOAM_PHASE_PROFILE_ENABLED ${profile})
  if(scenario)
    set(SOAM_SCENARIO_FORWARD_DECL
      "class ProductionTransitionEvaluatorScenarioAccess;")
    set(SOAM_MESH_SCENARIO_FRIEND
      "    friend class detail::ProductionTransitionEvaluatorScenarioAccess;")
    set(SOAM_HANDLE_SCENARIO_FRIEND
      "    friend class ProductionTransitionEvaluatorScenarioAccess;")
    set(SOAM_BINDING_ACCESS_SCENARIO_FRIEND
      "    friend class ProductionTransitionEvaluatorScenarioAccess;")
  else()
    set(SOAM_SCENARIO_FORWARD_DECL "")
    set(SOAM_MESH_SCENARIO_FRIEND "")
    set(SOAM_HANDLE_SCENARIO_FRIEND "")
    set(SOAM_BINDING_ACCESS_SCENARIO_FRIEND "")
  endif()
  file(MAKE_DIRECTORY "${output_dir}")
  configure_file(cmake/templates/system_architecture.hpp.in
    "${output_dir}/system_architecture.hpp" @ONLY)
  configure_file(cmake/templates/production_transition_evaluator.hpp.in
    "${output_dir}/production_transition_evaluator.hpp" @ONLY)
endfunction()

set(SOAM_NORMAL_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/normal")
set(SOAM_PROFILE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/profile")
set(SOAM_SCENARIO_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/scenario")
soam_configure_runtime_header("${SOAM_NORMAL_INCLUDE_DIR}" 0 0)
soam_configure_runtime_header("${SOAM_PROFILE_INCLUDE_DIR}" 1 0)
soam_configure_runtime_header("${SOAM_SCENARIO_INCLUDE_DIR}" 0 1)
target_include_directories(adaptive_mesh_domain BEFORE INTERFACE
  $<BUILD_INTERFACE:${SOAM_NORMAL_INCLUDE_DIR}>)

function(soam_add_runtime target include_dir)
  add_library(${target} STATIC src/system_architecture.cpp)
  target_link_libraries(${target} PUBLIC adaptive_mesh_domain Threads::Threads)
  target_include_directories(${target}
    PUBLIC $<BUILD_INTERFACE:${include_dir}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endfunction()

soam_add_runtime(adaptive_mesh "${SOAM_NORMAL_INCLUDE_DIR}")
add_library(AdaptiveMesh::adaptive_mesh ALIAS adaptive_mesh)
soam_add_runtime(adaptive_mesh_profile "${SOAM_PROFILE_INCLUDE_DIR}")
set_target_properties(adaptive_mesh_profile PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_library(adaptive_mesh_k12_live STATIC
  src/production_transition_evaluator.cpp
  src/production_transition_evaluator_live.cpp)
target_link_libraries(adaptive_mesh_k12_live PUBLIC adaptive_mesh)
target_include_directories(adaptive_mesh_k12_live PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src ${SOAM_NORMAL_INCLUDE_DIR})
add_library(AdaptiveMesh::adaptive_mesh_k12_live ALIAS adaptive_mesh_k12_live)

if(BUILD_TESTS)
  enable_testing()
  soam_add_runtime(adaptive_mesh_scenario "${SOAM_SCENARIO_INCLUDE_DIR}")
  set_target_properties(adaptive_mesh_scenario PROPERTIES EXCLUDE_FROM_ALL TRUE)
  add_library(adaptive_mesh_k12_scenario_support STATIC
    src/production_transition_evaluator.cpp
    src/production_transition_evaluator_live.cpp
    tests/support/production_transition_evaluator_live_scenarios.cpp)
  target_link_libraries(adaptive_mesh_k12_scenario_support PUBLIC adaptive_mesh_scenario)
  target_include_directories(adaptive_mesh_k12_scenario_support PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${SOAM_SCENARIO_INCLUDE_DIR})

  function(soam_add_test name source dependency)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${dependency})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME run_${name} COMMAND ${name})
  endfunction()
  soam_add_test(mesh_tests tests/mesh_tests.cpp adaptive_mesh)
  soam_add_test(topology_exception_safety_tests tests/topology_exception_safety_tests.cpp adaptive_mesh)
  set_tests_properties(run_topology_exception_safety_tests PROPERTIES TIMEOUT 60)
  soam_add_test(worker_pool_lifecycle_tests tests/worker_pool_lifecycle_tests.cpp adaptive_mesh)
  set_tests_properties(run_worker_pool_lifecycle_tests PROPERTIES TIMEOUT 30)
  soam_add_test(api_access_tests tests/api_access_tests.cpp adaptive_mesh)
  soam_add_test(production_transition_eligibility_tests tests/production_transition_eligibility_tests.cpp adaptive_mesh)
  soam_add_test(production_transition_eligibility_compile_tests tests/production_transition_eligibility_compile_tests.cpp adaptive_mesh_domain)
  soam_add_test(production_authority_ledger_tests tests/production_authority_ledger_tests.cpp adaptive_mesh)
  target_include_directories(production_authority_ledger_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  soam_add_test(production_persistence_tests tests/production_persistence_tests.cpp adaptive_mesh)
  target_include_directories(production_persistence_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  soam_add_test(production_transition_evaluator_tests tests/production_transition_evaluator_tests.cpp adaptive_mesh_k12_scenario_support)
  soam_add_test(production_transition_evaluator_compile_tests tests/production_transition_evaluator_compile_tests.cpp adaptive_mesh_domain)
  soam_add_test(production_transition_evaluator_concurrency_tests tests/production_transition_evaluator_concurrency_tests.cpp adaptive_mesh_k12_scenario_support)
  soam_add_test(production_transition_evaluator_live_binding_tests tests/production_transition_evaluator_live_binding_tests.cpp adaptive_mesh_k12_scenario_support)
  soam_add_test(production_transition_evaluator_live_concurrency_tests tests/production_transition_evaluator_live_concurrency_tests.cpp adaptive_mesh_k12_scenario_support)
  soam_add_test(production_transition_commit_tests tests/production_transition_commit_tests.cpp adaptive_mesh_k12_scenario_support)
  soam_add_test(production_transition_evaluator_live_compile_tests tests/production_transition_evaluator_live_compile_tests.cpp adaptive_mesh_k12_live)
  soam_add_test(production_runtime_consumer_tests tests/production_runtime_consumer_tests.cpp adaptive_mesh_k12_live)
  foreach(probe consumer_complete_privileged_type consumer_define_privileged_body
      consumer_construct_evaluator consumer_inject_backend consumer_include_scenario_surface
      consumer_force_scenario_macro b5_nc01_direct_capability_construction
      b5_nc02_capability_copy b5_nc03_eligibility_as_capability
      b5_nc04_fabricated_authority_context)
    add_test(NAME compile_fail_${probe} COMMAND ${CMAKE_COMMAND}
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/${probe}.cpp
      -DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include
      -DGENERATED_INCLUDE_DIR=${SOAM_NORMAL_INCLUDE_DIR}
      -DCXX=${CMAKE_CXX_COMPILER}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExpectCompileFailure.cmake)
  endforeach()
  add_test(NAME verify_adaptive_mesh_artifact_isolation
    COMMAND ${CMAKE_COMMAND}
      -DARTIFACT=$<TARGET_FILE:adaptive_mesh>
      -DNM=${CMAKE_NM}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyArtifactIsolation.cmake)
  add_test(NAME verify_k12_live_artifact_isolation
    COMMAND ${CMAKE_COMMAND}
      -DARTIFACT=$<TARGET_FILE:adaptive_mesh_k12_live>
      -DNM=${CMAKE_NM}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyArtifactIsolation.cmake)
endif()

if(BUILD_BENCHMARKS)
  add_executable(mesh_benchmark benchmarks/mesh_benchmark.cpp)
  target_link_libraries(mesh_benchmark PRIVATE adaptive_mesh)
  add_executable(mesh_scaling_sweep benchmarks/mesh_scaling_sweep.cpp)
  target_link_libraries(mesh_scaling_sweep PRIVATE adaptive_mesh)
  add_executable(mesh_phase_profile benchmarks/mesh_phase_profile.cpp)
  target_link_libraries(mesh_phase_profile PRIVATE adaptive_mesh_profile)
  target_include_directories(mesh_phase_profile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src ${SOAM_PROFILE_INCLUDE_DIR})
  add_executable(mesh_benchmark_profile benchmarks/mesh_benchmark.cpp)
  target_link_libraries(mesh_benchmark_profile PRIVATE adaptive_mesh_profile)
  target_include_directories(mesh_benchmark_profile PRIVATE ${SOAM_PROFILE_INCLUDE_DIR})
endif()
if(BUILD_EXAMPLES)
  add_executable(adaptive_mesh_demo examples/demo.cpp)
  target_link_libraries(adaptive_mesh_demo PRIVATE adaptive_mesh)
endif()

install(TARGETS adaptive_mesh_domain adaptive_mesh adaptive_mesh_k12_live
  EXPORT AdaptiveMeshTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES "${SOAM_NORMAL_INCLUDE_DIR}/system_architecture.hpp"
  "${SOAM_NORMAL_INCLUDE_DIR}/production_transition_evaluator.hpp"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT AdaptiveMeshTargets FILE AdaptiveMeshTargets.cmake
  NAMESPACE AdaptiveMesh:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaptiveMesh)
configure_package_config_file(cmake/AdaptiveMeshConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/AdaptiveMeshConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaptiveMesh)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/AdaptiveMeshConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/AdaptiveMeshConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/AdaptiveMeshConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdaptiveMesh)
