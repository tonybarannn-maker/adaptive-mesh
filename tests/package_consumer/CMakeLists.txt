cmake_minimum_required(VERSION 3.25)
project(AdaptiveMeshInstalledConsumer LANGUAGES CXX)

find_package(AdaptiveMesh 2.0 CONFIG REQUIRED)

add_executable(domain_consumer main.cpp)
target_compile_definitions(domain_consumer PRIVATE SOAM_DOMAIN_ONLY=1)
target_link_libraries(domain_consumer PRIVATE AdaptiveMesh::adaptive_mesh_domain)

add_executable(runtime_consumer main.cpp)
target_link_libraries(runtime_consumer PRIVATE AdaptiveMesh::adaptive_mesh)

add_executable(k12_live_consumer main.cpp)
target_link_libraries(k12_live_consumer PRIVATE AdaptiveMesh::adaptive_mesh_k12_live)
