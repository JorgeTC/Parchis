# project/test/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")

add_subdirectory(googletest)

# Set the desired runtime library setting for Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FILE(GLOB TestSourceFiles "${PROJECT_SOURCE_DIR}/test/*.cpp")

FILE(GLOB SrcSourceFiles "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM SrcSourceFiles "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Add your test source files
add_executable(test
    ${TestSourceFiles}
    ${SrcSourceFiles}
)

# Link with the testing framework and your project library
target_link_libraries(test PRIVATE gtest gtest_main)

# Include directories
target_include_directories(test PRIVATE "${PROJECT_SOURCE_DIR}/include")
