cmake_minimum_required(VERSION 3.10.2)
project(SharemapProject)

# find python with NO_CMAKE_PATH so we get the system cmake, not toolchain
find_program(PYTHON_EXECUTABLE python3 REQUIRED NO_CMAKE_PATH)
message(STATUS "PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}")

# ##############################################################################
# sharemap generation
# ##############################################################################
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sharemap.hpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sharemap_gen.py
          ${CMAKE_CURRENT_SOURCE_DIR}/sharemap.cpp.jinja
          ${CMAKE_CURRENT_SOURCE_DIR}/schema.yaml
  COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/sharemap_gen.py
    --template=${CMAKE_CURRENT_SOURCE_DIR}/sharemap.cpp.jinja
    --output=${CMAKE_CURRENT_BINARY_DIR}/sharemap.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/schema.yaml)

add_custom_target(sharemap_hpp DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sharemap.hpp)

# ##############################################################################
# build example
# ##############################################################################
add_subdirectory(sharemap_client)
