# project/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

project(parchis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")

# Set the desired runtime library setting for Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

FILE(GLOB SourceFiles ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add your source files
add_executable(parchis
    ${SourceFiles}
)

# Include directories
target_include_directories(parchis PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Include tests directory
add_subdirectory(test)
