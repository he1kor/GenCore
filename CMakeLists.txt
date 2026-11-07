cmake_minimum_required(VERSION 3.14)
project(GenCore)

set(CMAKE_CXX_STANDARD 23)

find_path(GENCORE_BOOST_INCLUDE_DIR boost/graph/boyer_myrvold_planar_test.hpp)
if(NOT GENCORE_BOOST_INCLUDE_DIR)
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(gencore_boost
        URL https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.bz2
        URL_HASH SHA256=af57be25cb4c4f4b413ed692fe378affb4352ea50fbe294a11ef548f4d527d89
    )
    FetchContent_GetProperties(gencore_boost)
    if(NOT gencore_boost_POPULATED)
        FetchContent_Populate(gencore_boost)
    endif()
    set(GENCORE_BOOST_INCLUDE_DIR "${gencore_boost_SOURCE_DIR}" CACHE PATH "Boost headers" FORCE)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(GenCore ${SOURCES})
target_include_directories(GenCore SYSTEM PRIVATE "${GENCORE_BOOST_INCLUDE_DIR}")

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
    add_executable(planar_embedding_regression tests/planar_embedding_regression.cc)
    target_link_libraries(planar_embedding_regression PRIVATE GenCore)
    add_test(NAME planar_embedding_regression COMMAND planar_embedding_regression)
endif()
