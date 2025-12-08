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

# Lexical Analyzer
add_executable(lexicalAnalyzer
        lexicalAnalyzer/lexical_analyzer.cpp
)

# Symbol Table Generator
add_executable(symbolTable
        symbolTable/symbol_table.cpp
)

# Parser
add_executable(parser
        parser/parser.cpp
)

# IR Generator
add_executable(irGenerator
        irGenerator/ir_generator.cpp
)
target_include_directories(irGenerator PRIVATE ${CMAKE_SOURCE_DIR}/parser)
target_link_libraries(irGenerator ${llvm_libs})

# ============================================
# CLANGAX - Main Compiler Driver
# ============================================
add_executable(clangax
        clangax.cpp
)

# ============================================
# COMPILER WARNINGS / OPTIMIZATIONS
# ============================================
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lexicalAnalyzer PRIVATE -Wall -Wextra -O2)
    target_compile_options(symbolTable PRIVATE -Wall -Wextra -O2)
    target_compile_options(parser PRIVATE -Wall -Wextra -O2)
    target_compile_options(irGenerator PRIVATE -Wall -Wextra -O2)
    target_compile_options(clangax PRIVATE -Wall -Wextra -O2)
endif()

# ============================================
# INSTALL TARGETS
# ============================================
install(TARGETS lexicalAnalyzer symbolTable parser irGenerator clangax
        RUNTIME DESTINATION bin
)

# ============================================
# CUSTOM TARGETS FOR CONVENIENCE
# ============================================

# Test target - compile and run SampleCode.cax
add_custom_target(test
        COMMAND ${CMAKE_COMMAND} -E echo "Converting SampleCode.txt to SampleCode.cax..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/SampleCode.txt
        ${CMAKE_BINARY_DIR}/SampleCode.cax
        COMMAND ${CMAKE_COMMAND} -E echo "Compiling SampleCode.cax..."
        COMMAND ${CMAKE_BINARY_DIR}/clangax ${CMAKE_BINARY_DIR}/SampleCode.cax -o test_program -v
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Running compiled program:"
        COMMAND ${CMAKE_COMMAND} -E echo "========================================"
        COMMAND ${CMAKE_BINARY_DIR}/test_program
        DEPENDS clangax lexicalAnalyzer parser symbolTable irGenerator
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Testing clangax compiler with SampleCode.cax"
)

# Quick test target - just compile, don't run
add_custom_target(test-compile
        COMMAND ${CMAKE_COMMAND} -E echo "Converting SampleCode.txt to SampleCode.cax..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/SampleCode.txt
        ${CMAKE_BINARY_DIR}/SampleCode.cax
        COMMAND ${CMAKE_COMMAND} -E echo "Compiling SampleCode.cax..."
        COMMAND ${CMAKE_BINARY_DIR}/clangax ${CMAKE_BINARY_DIR}/SampleCode.cax -o test_program -v
        DEPENDS clangax lexicalAnalyzer parser symbolTable irGenerator
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Testing clangax compiler (compile only)"
)

# Clean target for generated files
add_custom_target(clean-generated
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning generated files..."
        COMMAND ${CMAKE_COMMAND} -E remove -f
        ${CMAKE_BINARY_DIR}/lexical_report.txt
        ${CMAKE_BINARY_DIR}/parser/parse_report.txt
        ${CMAKE_BINARY_DIR}/parser/parse_tree.txt
        ${CMAKE_BINARY_DIR}/symbol_table.csv
        ${CMAKE_BINARY_DIR}/symbol_table_report.txt
        ${CMAKE_BINARY_DIR}/irGenerator/output.ll
        ${CMAKE_BINARY_DIR}/*.ll
        ${CMAKE_BINARY_DIR}/test_program
        ${CMAKE_BINARY_DIR}/SampleCode.cax
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Removing all generated compilation artifacts"
)

# Create example .cax file
add_custom_target(create-example
        COMMAND ${CMAKE_COMMAND} -E echo "Creating hello.cax example..."
        COMMAND ${CMAKE_COMMAND} -E echo "// hello.cax - Simple C-Accel program" > ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "#import \"Sample\"" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "func(Main) {" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "    print(\"Hello, C-Accel!\")" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "    print(\"Compilation successful!\")" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "}" >> ${CMAKE_BINARY_DIR}/hello.cax
        COMMAND ${CMAKE_COMMAND} -E echo "Created: ${CMAKE_BINARY_DIR}/hello.cax"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating example hello.cax file"
)

# Print build info
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  C-Accel Compiler (clangax) Build")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Executables will be built:")
message(STATUS "  - clangax (main driver)")
message(STATUS "  - lexicalAnalyzer")
message(STATUS "  - parser")
message(STATUS "  - symbolTable")
message(STATUS "  - irGenerator")
message(STATUS "")
message(STATUS "Custom targets available:")
message(STATUS "  make test           - Build and test with SampleCode.cax")
message(STATUS "  make test-compile   - Test compilation only (don't run)")
message(STATUS "  make clean-generated - Remove generated files")
message(STATUS "  make create-example - Create hello.cax example")
message(STATUS "========================================")
message(STATUS "")

# ============================================
# CLANGAX - Main Compiler Driver
# ============================================

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(clangax PRIVATE -Wall -Wextra -O2)
endif()

# Update install target to include clangax
install(TARGETS lexicalAnalyzer symbolTable parser irGenerator clangax
        RUNTIME DESTINATION bin
)