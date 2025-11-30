cmake_minimum_required(VERSION 3.10)
project(clangAX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# LLVM CONFIGURATION (Homebrew LLVM)
# ============================================
set(LLVM_DIR "/opt/homebrew/opt/llvm" CACHE PATH "Path to LLVM installation")

find_package(LLVM REQUIRED CONFIG
        PATHS ${LLVM_DIR}/lib/cmake/llvm
        NO_DEFAULT_PATH)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM Config: ${LLVM_DIR}")
message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM Library dirs: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM Definitions: ${LLVM_DEFINITIONS}")

# ============================================
# INCLUDE DIRECTORIES
# ============================================
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${LLVM_DIR}/lib/clang/19/include)   # <-- IMPORTANT
add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DLLVM_ENABLE_RTTI)                      # <-- REQUIRED for dynamic_cast

# ============================================
# LINK LLVM COMPONENTS
# ============================================
llvm_map_components_to_libnames(llvm_libs
        core
        irreader
        support
        analysis
        instcombine
        scalaropts
        native
        object
        orcjit
        runtimedyld
        mc
        mcparser
        bitreader
        bitwriter
        passes
        transformutils
        asmparser
)

# ============================================
# PROJECT INCLUDES
# ============================================
include_directories(${CMAKE_SOURCE_DIR})

# ============================================
# EXECUTABLES
# ============================================

add_executable(lexicalAnalyzer
        lexicalAnalyzer/lexical_analyzer.cpp
)

add_executable(symbolTable
        symbolTable/symbol_table.cpp
)

add_executable(parser
        parser/parser.cpp
)

add_executable(irGenerator
        irGenerator/ir_generator.cpp
)
target_include_directories(irGenerator PRIVATE ${CMAKE_SOURCE_DIR}/parser)
target_link_libraries(irGenerator ${llvm_libs})

# ============================================
# COMPILER WARNINGS / OPTIMIZATIONS
# ============================================
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lexicalAnalyzer PRIVATE -Wall -Wextra -O2)
    target_compile_options(symbolTable PRIVATE -Wall -Wextra -O2)
    target_compile_options(parser PRIVATE -Wall -Wextra -O2)
    target_compile_options(irGenerator PRIVATE -Wall -Wextra -O2)
endif()

# ============================================
# INSTALL TARGETS
# ============================================
install(TARGETS lexicalAnalyzer symbolTable parser irGenerator
        RUNTIME DESTINATION bin
)
