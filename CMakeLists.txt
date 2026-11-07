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
    add_executable(resource_mapping_regression tests/resource_mapping_regression.cc)
    target_link_libraries(resource_mapping_regression PRIVATE GenCore)
    add_test(NAME resource_mapping_regression COMMAND resource_mapping_regression)
    add_executable(magnetic_embedding_regression tests/magnetic_embedding_regression.cc)
    target_link_libraries(magnetic_embedding_regression PRIVATE GenCore)
    add_test(NAME magnetic_embedding_regression COMMAND magnetic_embedding_regression)
    add_executable(embedding_intersection_regression tests/embedding_intersection_regression.cc)
    target_link_libraries(embedding_intersection_regression PRIVATE GenCore)
    add_test(NAME embedding_intersection_regression COMMAND embedding_intersection_regression)
endif()
