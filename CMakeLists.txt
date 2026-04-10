cmake_minimum_required(VERSION 3.15)
project(ClangAX VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/common)

# =============================================================================
# COMMON LIBRARY (shared code)
# =============================================================================
add_library(clangax_common STATIC
        src/common/library.cpp
        src/common/gpu_detect.cpp
)

target_include_directories(clangax_common PUBLIC
        ${CMAKE_SOURCE_DIR}/src/common
)

# =============================================================================
# COMPILER EXECUTABLE (clangax)
# =============================================================================
add_executable(clangax
        src/compiler/clangax.cpp
        src/compiler/lexer.cpp
        src/compiler/parser.cpp
        src/compiler/bytecodeGen.cpp
)

target_include_directories(clangax PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/compiler
)

# Link with common library
target_link_libraries(clangax PRIVATE clangax_common)

# =============================================================================
# VIRTUAL MACHINE EXECUTABLE (caxvm)
# =============================================================================
add_executable(caxvm
        src/vm/caxvm.cpp
)

target_include_directories(caxvm PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
)

# Link with common library
target_link_libraries(caxvm PRIVATE clangax_common)

# Platform-specific libraries for dynamic loading (used by gpu_detect)
if(UNIX AND NOT APPLE)
    target_link_libraries(caxvm PRIVATE dl)
endif()

# =============================================================================
# DISASSEMBLER (caxdis)
# =============================================================================
add_executable(caxdis
        src/vm/caxdis.cpp
)

target_include_directories(caxdis PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
)

# Link with common library
target_link_libraries(caxdis PRIVATE clangax_common)

# =============================================================================
# INSTALL TARGETS
# =============================================================================
install(TARGETS clangax caxvm caxdis
        RUNTIME DESTINATION bin
)

# Optional: Install library headers for external use
install(FILES
        src/common/ast.h
        src/common/opcodes.h
        src/common/bytecode.h
        src/common/library.h
        src/common/gpu_detect.h
        DESTINATION include/clangax
)

# =============================================================================
# BUILD SUMMARY
# =============================================================================
message(STATUS "")
message(STATUS "ClangAX Toolchain Configuration")
message(STATUS "================================")
message(STATUS "Version:      ${PROJECT_VERSION}")
message(STATUS "Compiler:     clangax (NO LLVM DEPENDENCY)")
message(STATUS "VM:           caxvm")
message(STATUS "Disassembler: caxdis")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Standard Libraries:")
message(STATUS "  _math    - Mathematical operations")
message(STATUS "  _datastr - Data structures")
message(STATUS "  _alg     - Algorithms")
message(STATUS "  _ml      - Machine learning")
message(STATUS "")