#include "library.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace ClangAX {

void LibraryManager::initializeBuiltinLibraries() {
    // Register all built-in libraries
    libraries["_math"] = BuiltinLibraries::createMathLibrary();
    libraries["_datastr"] = BuiltinLibraries::createDatastrLibrary();
    libraries["_alg"] = BuiltinLibraries::createAlgLibrary();
    libraries["_ml"] = BuiltinLibraries::createMLLibrary();
}

std::shared_ptr<Library> LibraryManager::loadUserLibrary(const std::string& path) {
    // Read the .cax file
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open library: " << path << std::endl;
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Extract library name from filename
    std::filesystem::path p(path);
    std::string libName = p.stem().string();

    auto lib = std::make_shared<Library>(libName, LibraryType::USER);

    // Simple parsing: look for func() = "name" patterns
    // This is a simplified parser - in a real implementation you'd use the full Lexer/Parser

    size_t pos = 0;
    while (pos < source.length()) {
        // Find "func"
        size_t funcPos = source.find("func", pos);
        if (funcPos == std::string::npos) break;

        // Move past "func()"
        pos = funcPos + 4;

        // Skip whitespace and parentheses
        while (pos < source.length() && (std::isspace(source[pos]) || source[pos] == '(' || source[pos] == ')')) {
            pos++;
        }

        // Check for = "name"
        if (pos < source.length() && source[pos] == '=') {
            pos++;

            // Skip whitespace
            while (pos < source.length() && std::isspace(source[pos])) {
                pos++;
            }

            // Extract function name from string
            if (pos < source.length() && source[pos] == '"') {
                pos++;
                size_t nameStart = pos;
                while (pos < source.length() && source[pos] != '"') {
                    pos++;
                }

                if (pos < source.length()) {
                    std::string funcName = source.substr(nameStart, pos - nameStart);

                    // Create a library function entry
                    auto func = std::make_shared<LibraryFunction>(funcName, libName, false);
                    lib->addFunction(func);

                    std::cout << "  Found function: " << funcName << std::endl;
                }
            }
        }
    }

    return lib;
}

std::shared_ptr<Library> LibraryManager::importLibrary(const std::string& name) {
    // Check if already loaded
    if (isLibraryLoaded(name)) {
        return libraries[name];
    }

    // Check if it's a built-in library
    if (name[0] == '_') {
        // Built-in libraries should already be initialized
        auto it = libraries.find(name);
        if (it != libraries.end()) {
            return it->second;
        } else {
            std::cerr << "Unknown built-in library: " << name << std::endl;
            return nullptr;
        }
    }

    // User library - construct path
    std::string path = name;
    if (path.find(".cax") == std::string::npos) {
        path += ".cax";
    }

    // If stdlibPath is set, try there first
    if (!stdlibPath.empty()) {
        std::string fullPath = stdlibPath + "/" + path;
        if (std::filesystem::exists(fullPath)) {
            path = fullPath;
        }
    }

    // Check if file exists in current directory
    if (!std::filesystem::exists(path)) {
        std::cerr << "Library file not found: " << path << std::endl;
        return nullptr;
    }
    
    // Load and parse the library
    auto lib = loadUserLibrary(path);
    if (lib) {
        libraries[name] = lib;
    }
    
    return lib;
}

} // namespace ClangAX