cmake_minimum_required(VERSION 3.10)
project(clangAX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

# Lexical Analyzer Executable
add_executable(lexicalAnalyzer
        lexicalAnalyzer/lexical_analyzer.cpp
)

# Symbol Table Executable
add_executable(symbolTable
        symbolTable/symbol_table.cpp
)

# Parser Executable
add_executable(parser
        parser/parser.cpp
)

# Optional: warnings & optimizations
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lexicalAnalyzer PRIVATE -Wall -Wextra -O2)
    target_compile_options(symbolTable PRIVATE -Wall -Wextra -O2)
    target_compile_options(parser PRIVATE -Wall -Wextra -O2)
endif()