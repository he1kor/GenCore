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