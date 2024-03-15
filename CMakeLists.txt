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

# Write -DRUN_IWYU=ON on calling cmake to run iwyu
option(RUN_IWYU "Run IWYU analysis" OFF)
if (RUN_IWYU)
    find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
    if(iwyu_path)
        message("Found iwyu in ${iwyu_path}")
        set_property(TARGET parchis PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
    endif()
else()
    message("IWYU is set to false")
endif()

# Write DRUN_CLANG_TIDY=ON on calling cmake to run clang tidy
option(RUN_CLANG_TIDY "Run CLANG_TIDY analysis" OFF)
if (RUN_CLANG_TIDY)
    find_program(clang_tidy_path NAMES "clang-tidy" REQUIRED)
    if (clang_tidy_path)
        message("Found clang_tidy in ${clang_tidy_path}")
        set(CLANG_TIDY_COMMAND "${clang_tidy_path}" "-checks=-*,bugprone-*,cppcoreguidelines-*,clang-analyzer-*")
        set_property(TARGET parchis PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
else()
    message("CLANG_TIDY is set to false")
endif()
