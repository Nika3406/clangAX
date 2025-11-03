cmake_minimum_required(VERSION 3.10)
project(clangAX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

# Lexical Analyzer Executable
add_executable(lexicalAnalyzer
        lexicalAnalyzer/lexical_analyzer.cpp
)

# Optional: warnings & optimizations
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lexicalAnalyzer PRIVATE -Wall -Wextra -O2)
endif()
