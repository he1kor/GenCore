file(GLOB_RECURSE SOURCES 
    "*.cpp"
)

cmake_minimum_required(VERSION 3.8)
project(GenCore)

set(CMAKE_CXX_STANDARD 23)

add_library(GenCore ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

option(GENCORE_BUILD_TESTS "Build GenCore regression tests" OFF)
if(GENCORE_BUILD_TESTS)
    enable_testing()
    add_executable(blender_regression tests/blender_regression.cc)
    target_link_libraries(blender_regression PRIVATE GenCore)
    add_test(NAME blender_regression COMMAND blender_regression)
endif()
