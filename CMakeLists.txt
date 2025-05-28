cmake_minimum_required(VERSION 3.8)
project(OGDF_Example)

set(CMAKE_CXX_STANDARD 17)

# Build OGDF
include(ExternalProject)
ExternalProject_Add(OGDF_BUILD
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/OGDF
    BINARY_DIR ${CMAKE_BINARY_DIR}/ogdf-build
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/ogdf-install
        -G "MinGW Makefiles"
    INSTALL_COMMAND ""
)

# Import OGDF targets
add_library(OGDF_LIB INTERFACE)
add_dependencies(OGDF_LIB OGDF_BUILD)

target_include_directories(OGDF_LIB INTERFACE
    ${CMAKE_SOURCE_DIR}/libs/OGDF/include
    ${CMAKE_BINARY_DIR}/ogdf-build/include
)

target_link_directories(OGDF_LIB INTERFACE
    ${CMAKE_BINARY_DIR}/ogdf-build
)

target_link_libraries(OGDF_LIB INTERFACE
    -lOGDF
    -lCOIN
)

# Your executable
add_executable(ogdf_example src/main.cpp)
target_link_libraries(ogdf_example PRIVATE OGDF_LIB)