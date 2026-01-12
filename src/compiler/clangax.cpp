// clangax.cpp - Complete compiler with library support
#include "parser.cpp"
#include "bytecodeGen.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

using namespace ClangAX;

static void printUsage(const char* programName) {
    std::cout
        << "ClangAX Compiler v2.0 (with Libraries)\n"
        << "Usage: " << programName << " <source.cax> [options]\n\n"
        << "Options:\n"
        << "  -o <name>       Specify output filename base (default: output)\n"
        << "  -L <path>       Specify library search path\n"
        << "  --libs          List available built-in libraries\n"
        << "  --verbose       Print compilation stages\n"
        << "  -h, --help      Show this help message\n"
        << "  -v, --version   Show compiler version\n\n"
        << "Built-in Libraries:\n"
        << "  _math           Mathematical functions\n"
        << "  _datastr        Data structures\n"
        << "  _alg            Algorithms\n"
        << "  _ml             Machine learning\n\n"
        << "Output:\n"
        << "  Generates <name>.caxb bytecode file\n"
        << "  Run with: caxvm <name>.caxb\n";
}

static void printVersion() {
    std::cout << "ClangAX Compiler v1.1 (Library Support)\n";
}

static void printLibraries() {
    std::cout << "\n=== Built-in Libraries ===\n\n";

    std::cout << "_math - Mathematical Functions:\n";
    std::cout << "  quad, sqrt, pow, abs, sin, cos, tan, exp, log\n";
    std::cout << "  floor, ceil, round, min, max, gcd, lcm, factorial\n";
    std::cout << "  mean, median, stddev, variance, and more...\n\n";

    std::cout << "_datastr - Data Structures:\n";
    std::cout << "  Stack, Queue, Heap, Map, Set, Graph, Tree\n\n";

    std::cout << "_alg - Algorithms:\n";
    std::cout << "  Sorting: quicksort, mergesort, heapsort\n";
    std::cout << "  Search: binary_search, linear_search\n";
    std::cout << "  Graph: bfs, dfs, dijkstra\n\n";

    std::cout << "_ml - Machine Learning:\n";
    std::cout << "  Regression: linear_fit, poly_fit, logistic_fit\n";
    std::cout << "  Classification: knn_fit, dtree_fit\n";
    std::cout << "  Clustering: kmeans_fit\n";
    std::cout << "  Neural Networks: nn_new, nn_train\n\n";
}

static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static inline void vlog(bool verbose, const std::string& stage, const std::string& msg) {
    if (verbose) {
        std::cout << "[" << stage << "] " << msg << "\n";
    }
}

static inline void errorOut(const std::string& msg) {
    std::cerr << "ERROR: " << msg << "\n";
}

// Update the compileSource function in clangax.cpp

static bool compileSource(const std::string& inputFile,
                          const std::string& outputBase,
                          const std::string& libPath,
                          bool verbose) {
    try {
        vlog(verbose, "INPUT", "Reading source file: " + inputFile);
        std::string source = readFile(inputFile);
        if (source.empty()) {
            errorOut("Source file is empty: " + inputFile);
            return false;
        }

        vlog(verbose, "LEXER", "Tokenizing source code...");
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        vlog(verbose, "LEXER", "Tokenization complete (" + std::to_string(tokens.size()) + " tokens)");

        vlog(verbose, "PARSER", "Building Abstract Syntax Tree...");
        Parser parser(tokens);

        if (!libPath.empty()) {
            parser.setLibraryPath(libPath);
            vlog(verbose, "PARSER", "Library search path: " + libPath);
        }

        auto ast = parser.parse();
        if (!ast) {
            errorOut("Failed to build AST");
            return false;
        }
        vlog(verbose, "PARSER", "AST built successfully");

        // ===== NEW: Parse user library files =====
        auto userLibPaths = parser.getUserLibraryPaths();
        std::vector<std::shared_ptr<ASTNode>> userLibASTs;

        for (const auto& libPath : userLibPaths) {
            vlog(verbose, "LIBRARY", "Parsing user library: " + libPath);

            std::string libSource = readFile(libPath);
            if (libSource.empty()) {
                std::cerr << "Warning: Library file is empty: " << libPath << std::endl;
                continue;
            }

            Lexer libLexer(libSource);
            auto libTokens = libLexer.tokenize();

            Parser libParser(libTokens);
            auto libAST = libParser.parse();

            if (libAST) {
                userLibASTs.push_back(libAST);
                vlog(verbose, "LIBRARY", "Successfully parsed: " + libPath);
            }
        }

        vlog(verbose, "BYTECODE", "Generating bytecode...");
        BytecodeGenerator generator;

        // Pass imported libraries to generator
        generator.setImportedLibraries(parser.getImportedLibraries());
        generator.setUserLibraryFunctions(parser.getUserLibraryFunctions());

        // ===== NEW: Merge user library functions into the main program =====
        // Add all user library functions to the main AST
        for (auto& libAST : userLibASTs) {
            if (libAST->type == NodeType::PROGRAM) {
                for (auto& child : libAST->children) {
                    if (child->type == NodeType::FUNCTION_DECL) {
                        // Add this function to the main program
                        ast->addChild(child);
                    }
                }
            }
        }

        // Generate bytecode for everything (main + libraries)
        generator.generateProgram(ast);

        const std::string outputPath = outputBase + ".caxb";
        generator.writeBytecode(outputPath);
        vlog(verbose, "OUTPUT", "Wrote bytecode: " + outputPath);

        return true;

    } catch (const std::exception& e) {
        errorOut(e.what());
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Handle special flags early
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        }
        if (arg == "--libs") {
            printLibraries();
            return 0;
        }
    }

    if (argc < 2) {
        errorOut("No input file specified (run --help for usage).");
        return 1;
    }

    std::string inputFile;
    std::string outputBase = "output";
    std::string libPath;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                outputBase = argv[++i];
            } else {
                errorOut("Missing argument for -o");
                return 1;
            }
        } else if (arg == "-L") {
            if (i + 1 < argc) {
                libPath = argv[++i];
            } else {
                errorOut("Missing argument for -L");
                return 1;
            }
        } else if (!arg.empty() && arg[0] != '-') {
            inputFile = arg;
        } else {
            errorOut("Unknown option: " + arg + " (run --help for usage).");
            return 1;
        }
    }

    if (inputFile.empty()) {
        errorOut("No input file specified (run --help for usage).");
        return 1;
    }

    const bool ok = compileSource(inputFile, outputBase, libPath, verbose);
    return ok ? 0 : 1;
}