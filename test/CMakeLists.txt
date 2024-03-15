# project/test/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")

add_subdirectory(googletest)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

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

# Write -DRUN_IWYU=ON on calling cmake to run iwyu
option(RUN_IWYU "Run IWYU analysis" OFF)
if (RUN_IWYU)
    find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
    if(iwyu_path)
        message("Found iwyu in ${iwyu_path}")
        set_property(TARGET test PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
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
        set_property(TARGET test PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
else()
    message("CLANG_TIDY is set to false")
endif()
