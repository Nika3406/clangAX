#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sstream>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;
using namespace std;

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

class ClangaxCompiler {
private:
    string inputFile;
    string outputFile;
    string outputLL;
    bool verbose;
    bool keepIntermediate;
    int optimizeLevel;

    string findExecutable(const string& name) {
        // Search in cmake-build-debug first (your CLion default), then build
        vector<string> searchPaths = {
            "./cmake-build-debug/" + name,
            "./build/" + name,
            "./" + name  // Also check current directory
        };

        for (const auto& path : searchPaths) {
            if (fs::exists(path)) {
                return path;
            }
        }

        return "";
    }

    void printBanner() {
        cout << CYAN << BOLD;
        cout << "╔═══════════════════════════════════════╗\n";
        cout << "║         CLANGAX COMPILER v2.0         ║\n";
        cout << "║       C-Accel to Native Compiler      ║\n";
        cout << "╚═══════════════════════════════════════╝\n";
        cout << RESET << "\n";
    }

    void printStep(const string& step, const string& message) {
        cout << BLUE << "[" << step << "]" << RESET << " " << message << endl;
    }

    void printSuccess(const string& message) {
        cout << GREEN << " " << message << RESET << endl;
    }

    void printError(const string& message) {
        cerr << RED << " ERROR: " << message << RESET << endl;
    }

    void printWarning(const string& message) {
        cout << YELLOW << " WARNING: " << message << RESET << endl;
    }

    bool validateInputFile() {
        if (!fs::exists(inputFile)) {
            printError("Input file does not exist: " + inputFile);
            return false;
        }

        string ext = fs::path(inputFile).extension().string();
        if (ext != ".cax" && ext != ".txt") {
            printWarning("Input file has unusual extension: " + ext);
            cout << "  Expected .cax or .txt" << endl;
        }

        return true;
    }

    bool runIRGenerator() {
        printStep("IR GEN", "Generating LLVM IR...");

        string irGenPath = findExecutable("irGenerator");

        if (irGenPath.empty()) {
            printError("Could not find irGenerator executable");
            printWarning("Please build the project first with CLion or: cmake --build cmake-build-debug");
            return false;
        }

        // Pass the input file as argument to irGenerator
        string cmd = irGenPath + " " + inputFile;

        if (verbose) {
            cout << "  Command: " << cmd << endl;
        }

        int result = system(cmd.c_str());

        if (result != 0) {
            printError("IR generation failed");
            return false;
        }

        // Check for output.ll in irGenerator folder
        if (!fs::exists("irGenerator/output.ll")) {
            printError("IR generator did not produce output.ll");
            return false;
        }

        // Copy output.ll to desired location if specified
        if (!outputLL.empty() && outputLL != "irGenerator/output.ll") {
            try {
                fs::copy_file("irGenerator/output.ll", outputLL,
                             fs::copy_options::overwrite_existing);
                printSuccess("LLVM IR generated: " + outputLL);
            } catch (const exception& e) {
                printWarning("Could not copy output.ll to " + outputLL);
                outputLL = "irGenerator/output.ll";
                printSuccess("LLVM IR generated: " + outputLL);
            }
        } else {
            outputLL = "irGenerator/output.ll";
            printSuccess("LLVM IR generated: " + outputLL);
        }

        return true;
    }

    bool compileToExecutable() {
        printStep("COMPILE", "Compiling to native executable...");

        // Use clang to compile LLVM IR to executable
        stringstream cmd;
        cmd << "clang " << outputLL << " -o " << outputFile;

        if (optimizeLevel > 0) {
            cmd << " -O" << optimizeLevel;
        }

        if (verbose) {
            cout << "  Command: " << cmd.str() << endl;
        }

        int result = system(cmd.str().c_str());

        if (result != 0) {
            printError("Compilation to executable failed");
            printWarning("Make sure clang is installed and in your PATH");
            return false;
        }

        if (fs::exists(outputFile)) {
            printSuccess("Executable created: " + outputFile);

            // Make executable on Unix-like systems
            #ifndef _WIN32
            chmod(outputFile.c_str(), 0755);
            #endif

            return true;
        }

        printError("Executable was not created");
        return false;
    }

    void cleanup() {
        if (!keepIntermediate) {
            printStep("CLEANUP", "Removing intermediate files...");

            if (outputLL != "irGenerator/output.ll") {
                if (fs::exists(outputLL)) {
                    fs::remove(outputLL);
                    if (verbose) {
                        cout << "  Removed: " << outputLL << endl;
                    }
                }
            }

            printSuccess("Cleanup completed");
        }
    }

    void printUsage(const char* progName) {
        cout << "Usage: " << progName << " <input.cax> [options]\n\n";
        cout << "Options:\n";
        cout << "  -o <file>          Specify output executable name\n";
        cout << "  -emit-llvm <file>  Output LLVM IR to specified file\n";
        cout << "  -v, --verbose      Enable verbose output\n";
        cout << "  -k, --keep         Keep intermediate files\n";
        cout << "  -O<level>          Optimization level (0-3)\n";
        cout << "  -h, --help         Show this help message\n\n";
        cout << "Examples:\n";
        cout << "  " << progName << " program.cax\n";
        cout << "  " << progName << " program.cax -o myprogram\n";
        cout << "  " << progName << " program.cax -emit-llvm output.ll -k\n";
        cout << "  " << progName << " program.cax -v -O2\n";
    }

public:
    ClangaxCompiler()
        : verbose(false), keepIntermediate(false),
          optimizeLevel(0), outputLL("") {}

    bool parseArguments(int argc, char* argv[]) {
        if (argc < 2) {
            printUsage(argv[0]);
            return false;
        }

        for (int i = 1; i < argc; i++) {
            string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                return false;
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "-k" || arg == "--keep") {
                keepIntermediate = true;
            } else if (arg == "-o" && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (arg == "-emit-llvm" && i + 1 < argc) {
                outputLL = argv[++i];
                keepIntermediate = true;
            } else if (arg.substr(0, 2) == "-O" && arg.length() == 3) {
                optimizeLevel = arg[2] - '0';
            } else if (inputFile.empty()) {
                inputFile = arg;
            } else {
                printError("Unknown argument: " + arg);
                return false;
            }
        }

        if (inputFile.empty()) {
            printError("No input file specified");
            return false;
        }

        // Set default output file if not specified
        if (outputFile.empty()) {
            fs::path p(inputFile);
            outputFile = p.stem().string();
        }

        return true;
    }

    bool compile() {
        printBanner();

        // Validate input
        if (!validateInputFile()) {
            return false;
        }

        printStep("INPUT", "Source file: " + inputFile);
        if (verbose) {
            cout << "  Output: " << outputFile << endl;
            if (!outputLL.empty()) {
                cout << "  LLVM IR: " << outputLL << endl;
            }
        }
        cout << endl;

        // Step 1: IR Generation (runs lexer, parser, symbol table internally)
        if (!runIRGenerator()) {
            return false;
        }

        // Step 2: Compile to Executable
        if (!compileToExecutable()) {
            return false;
        }

        // Step 3: Cleanup
        cleanup();

        cout << endl;
        cout << GREEN << BOLD << "╔═══════════════════════════════════════╗\n";
        cout << "║         COMPILATION SUCCESSFUL!       ║\n";
        cout << "╚═══════════════════════════════════════╝" << RESET << endl;
        cout << endl;
        cout << "Run your program with: " << CYAN << "./" << outputFile << RESET << endl;
        cout << endl;

        return true;
    }
};

int main(int argc, char* argv[]) {
    ClangaxCompiler compiler;

    if (!compiler.parseArguments(argc, argv)) {
        return 1;
    }

    if (!compiler.compile()) {
        cerr << endl;
        cerr << RED << "Compilation failed." << RESET << endl;
        return 1;
    }

    return 0;
}