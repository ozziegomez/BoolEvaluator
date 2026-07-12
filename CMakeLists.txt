
cmake_minimum_required(VERSION 4.3.4)

# The essential magic string to unlock experimental 'import std;' in CMake 4.3+
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

project(booleval VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create the executable
add_executable(booleval)

# Necessary so std module also compiles with /utf-8
if(MSVC)
    add_compile_options(/utf-8 /std:c++latest)
endif()

# Use FILE_SET to handle your modern C++ modules
target_sources(booleval
    PRIVATE
        main.cpp
    PUBLIC FILE_SET CXX_MODULES FILES
        Tokens.ixx
        Truth_table.ixx
)

# Enable 'import std' for this specific target
set_target_properties(booleval PROPERTIES CXX_MODULE_STD ON)

# Enable aggressive compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(booleval PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(booleval PRIVATE /W4 /utf-8 /std:c++latest /permissive- /Zc:__cplusplus)
endif()