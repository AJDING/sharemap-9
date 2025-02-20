cmake_minimum_required(VERSION 3.10.2)
project(SharemapClientProject)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sharemap_client.cpp
  DEPENDS ${PROJECT_SOURCE_DIR}/../sharemap_gen.py
          ${PROJECT_SOURCE_DIR}/sharemap_client.cpp.jinja
          ${PROJECT_SOURCE_DIR}/../schema.yaml
  COMMAND
    ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/../sharemap_gen.py
    --template=${PROJECT_SOURCE_DIR}/sharemap_client.cpp.jinja
    --output=${CMAKE_CURRENT_BINARY_DIR}/sharemap_client.cpp
    ${PROJECT_SOURCE_DIR}/../schema.yaml)

add_custom_target(sharemap_client_cpp
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sharemap_client.cpp)

add_executable(sharemap_client)
target_include_directories(sharemap_client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(sharemap_client
               PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/sharemap_client.cpp)

# threading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(sharemap_client PRIVATE Threads::Threads)

# Sharemap header file
target_include_directories(sharemap_client PRIVATE ${PROJECT_BINARY_DIR}/..)
add_dependencies(sharemap_client sharemap_hpp)

# header only udp
target_include_directories(sharemap_client PRIVATE ${PROJECT_SOURCE_DIR})
add_executable(test_udp_socket test_udp_socket.cpp)
