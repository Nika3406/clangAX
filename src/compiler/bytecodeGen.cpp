#ifndef CLANGAX_BYTECODE_GEN_H
#define CLANGAX_BYTECODE_GEN_H

#include "../common/ast.h"
#include "../common/opcodes.h"
#include "../common/bytecode.h"
#include "../common/library.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

namespace ClangAX {

class ConstantPool {
private:
    std::vector<Constant> constants;
    std::map<std::string, uint32_t> stringLookup;
    std::map<int32_t, uint32_t> intLookup;
    std::map<float, uint32_t> floatLookup;

public:
    uint32_t addString(const std::string& str) {
        auto it = stringLookup.find(str);
        if (it != stringLookup.end()) return it->second;

        Constant c;
        c.type = ConstantType::STRING;
        c.strValue = str;

        uint32_t idx = constants.size();
        constants.push_back(c);
        stringLookup[str] = idx;
        return idx;
    }

    uint32_t addInt(int32_t val) {
        auto it = intLookup.find(val);
        if (it != intLookup.end()) return it->second;

        Constant c;
        c.type = ConstantType::INTEGER;
        c.value.i = val;

        uint32_t idx = constants.size();
        constants.push_back(c);
        intLookup[val] = idx;
        return idx;
    }

    uint32_t addFloat(float val) {
        auto it = floatLookup.find(val);
        if (it != floatLookup.end()) return it->second;

        Constant c;
        c.type = ConstantType::FLOAT;
        c.value.f = val;

        uint32_t idx = constants.size();
        constants.push_back(c);
        floatLookup[val] = idx;
        return idx;
    }

    const std::vector<Constant>& getConstants() const {
        return constants;
    }
};

class BytecodeGenerator {
private:
    std::vector<std::string> userLibraryFunctions;

    std::map<std::string, std::shared_ptr<Library>> importedLibraries;

    ConstantPool constantPool;
    std::vector<FunctionBytecode> functions;
    uint32_t entryPoint;

    std::map<std::string, uint32_t> funcIndex;
    std::string currentFuncType;

    BytecodeWriter writer;
    std::map<std::string, uint16_t> localVariables;

    uint16_t nextLocalIndex;
    uint16_t currentStackDepth;
    uint16_t maxStackDepth;

    void resetFunctionState() {
        localVariables.clear();
        nextLocalIndex = 0;
        currentStackDepth = 0;
        maxStackDepth = 0;
        writer.clear();
    }

    uint16_t getOrCreateLocal(const std::string& name) {
        auto it = localVariables.find(name);
        if (it != localVariables.end()) return it->second;

        uint16_t idx = nextLocalIndex++;
        localVariables[name] = idx;
        return idx;
    }

    void pushStack() {
        currentStackDepth++;
        if (currentStackDepth > maxStackDepth)
            maxStackDepth = currentStackDepth;
    }

    void popStack(int count = 1) {
        currentStackDepth -= count;
    }

public:
    BytecodeGenerator() : entryPoint(0) {}

    void setImportedLibraries(const std::map<std::string, std::shared_ptr<Library>>& libs) {
        importedLibraries = libs;
    }

    // Helper to generate a simple library call
    void generateSimpleLibCall(const std::string& opName, size_t argc) {
        uint32_t opIdx = constantPool.addString(opName);
        writer.writeByte(EXEC);
        writer.writeInt(opIdx);
        writer.writeShort(static_cast<uint16_t>(argc));
        writer.writeShort(0);  // 0 named args
    }

    void writeBytecode(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open output file: " << filename << std::endl;
            return;
        }

        // --- NEW: Pre-intern function names BEFORE writing constant pool ---
        std::vector<uint32_t> funcNameIdx(functions.size());
        for (size_t i = 0; i < functions.size(); i++) {
            funcNameIdx[i] = constantPool.addString(functions[i].name);
        }

        // Magic number
        file.write(reinterpret_cast<const char*>(&CAXB_MAGIC), sizeof(CAXB_MAGIC));

        // Version
        file.write(reinterpret_cast<const char*>(&CAXB_VERSION), sizeof(CAXB_VERSION));

        // Constant pool
        const auto& constants = constantPool.getConstants();
        uint32_t poolSize = constants.size();
        file.write(reinterpret_cast<const char*>(&poolSize), sizeof(poolSize));

        for (const auto& c : constants) {
            uint8_t type = static_cast<uint8_t>(c.type);
            file.write(reinterpret_cast<const char*>(&type), sizeof(type));
            switch (c.type) {
                case ConstantType::INTEGER:
                    file.write(reinterpret_cast<const char*>(&c.value.i), sizeof(int32_t));
                    break;
                case ConstantType::FLOAT:
                    file.write(reinterpret_cast<const char*>(&c.value.f), sizeof(float));
                    break;
                case ConstantType::STRING: {
                    uint32_t len = c.strValue.length();
                    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
                    file.write(c.strValue.data(), len);
                    break;
                }
                case ConstantType::DOUBLE:
                    file.write(reinterpret_cast<const char*>(&c.value.d), sizeof(double));
                    break;
            }
        }

        // Functions
        uint32_t funcCount = functions.size();
        file.write(reinterpret_cast<const char*>(&funcCount), sizeof(funcCount));

        for (size_t i = 0; i < functions.size(); i++) {
            const auto& func = functions[i];
            uint32_t nameIdx = funcNameIdx[i]; // now valid
            file.write(reinterpret_cast<const char*>(&nameIdx), sizeof(nameIdx));
            file.write(reinterpret_cast<const char*>(&func.localCount), sizeof(func.localCount));
            file.write(reinterpret_cast<const char*>(&func.maxStackDepth), sizeof(func.maxStackDepth));

            uint32_t codeLen = func.code.size();
            file.write(reinterpret_cast<const char*>(&codeLen), sizeof(codeLen));
            file.write(reinterpret_cast<const char*>(func.code.data()), codeLen);
        }

        // Entry point
        file.write(reinterpret_cast<const char*>(&entryPoint), sizeof(entryPoint));

        file.close();
    }



    void generateProgram(std::shared_ptr<ASTNode> ast) {
        if (!ast || ast->type != NodeType::PROGRAM) return;

        // Pass 1: reserve function slots so calls can reference forward decls.
        functions.clear();
        funcIndex.clear();
        for (auto& child : ast->children) {
            if (child->type != NodeType::FUNCTION_DECL) continue;
            std::string name = child->value;
            if (child->getAttribute("type") == "Main") name = "main";

            FunctionBytecode stub;
            stub.name = name;
            stub.localCount = 0;
            stub.maxStackDepth = 0;
            functions.push_back(stub);
            funcIndex[name] = static_cast<uint32_t>(functions.size() - 1);
        }

        // Pass 2: generate bodies
        uint32_t idx = 0;
        for (auto& child : ast->children) {
            if (child->type != NodeType::FUNCTION_DECL) continue;
            generateFunction(child, idx);
            idx++;
        }

        // Entry point
        auto it = funcIndex.find("main");
        if (it != funcIndex.end()) entryPoint = it->second;
    }

    void generateFunction(std::shared_ptr<ASTNode> node, uint32_t slot) {
        resetFunctionState();

        std::string funcName = node->value;
        if (node->getAttribute("type") == "Main") funcName = "main";
        currentFuncType = node->getAttribute("func_type");

        if (!node->children.empty() && node->children[0]->type == NodeType::BLOCK)
            generateBlock(node->children[0]);

        writer.writeByte(RET);

        FunctionBytecode& func = functions.at(slot);
        func.name = funcName;
        func.localCount = nextLocalIndex;
        func.maxStackDepth = maxStackDepth;
        func.code = writer.getCode();
    }

    void generateBlock(std::shared_ptr<ASTNode> node) {
        for (auto& stmt : node->children)
            generateStatement(stmt);
    }

    void generatePostIncrement(std::shared_ptr<ASTNode> node) {
        // idx++
        uint16_t localIdx = getOrCreateLocal(node->value);

        writer.writeByte(LOAD);
        writer.writeShort(localIdx);
        pushStack();

        uint32_t oneIdx = constantPool.addInt(1);
        writer.writeByte(LDC);
        writer.writeInt(oneIdx);
        pushStack();

        writer.writeByte(ADD);
        popStack(); // two -> one

        writer.writeByte(STORE);
        writer.writeShort(localIdx);
        popStack();
    }


    void generateStatement(std::shared_ptr<ASTNode> node) {
        switch (node->type) {
            case NodeType::ASSIGNMENT: generateAssignment(node); break;
            case NodeType::IF_STMT: generateIf(node); break;
            case NodeType::WHILE_STMT: generateWhile(node); break;
            case NodeType::FOR_STMT: generateFor(node); break;
            case NodeType::RETURN_STMT: generateReturn(node); break;
            case NodeType::PRINT_STMT: generatePrint(node); break;
            case NodeType::BLOCK: generateBlock(node); break;
            case NodeType::POST_INCREMENT: generatePostIncrement(node); break;
            case NodeType::CALL_EXPR: {
                bool leavesValue = generateFunctionCall(node);
                if (leavesValue) {
                    writer.writeByte(POP);
                    popStack();
                }
                break;
            }
            case NodeType::EXPRESSION_STMT: {
                if (node->children.empty()) break;

                auto expr = node->children[0];

                // For now, only allow function calls as expression-statements
                if (expr->type != NodeType::CALL_EXPR) {
                    throw std::runtime_error("Only function calls allowed as expression statements (for now).");
                }

                bool leavesValue = generateFunctionCall(expr);
                if (leavesValue) {
                    writer.writeByte(POP);
                    popStack();
                }
                break;
            }
            default: break;
        }
    }

    void generateAssignment(std::shared_ptr<ASTNode> node) {
        if (node->children.empty()) return;

        auto rhs = node->children[0];
        generateExpression(rhs);

        uint16_t localIdx = getOrCreateLocal(node->value);
        writer.writeByte(STORE);
        writer.writeShort(localIdx);
        popStack();
    }


    void generateExpression(std::shared_ptr<ASTNode> node) {
        switch (node->type) {
            case NodeType::LITERAL: generateLiteral(node); break;
            case NodeType::IDENTIFIER: generateIdentifier(node); break;
            case NodeType::BINARY_OP: generateBinaryOp(node); break;
            case NodeType::UNARY_OP: generateUnaryOp(node); break;
            case NodeType::CALL_EXPR: (void)generateFunctionCall(node); break;
            case NodeType::ARRAY_ACCESS: generateArrayAccess(node); break;
            case NodeType::ARRAY_LITERAL: generateArrayLiteral(node); break;
            default: break;
        }
    }

    void generateLiteral(std::shared_ptr<ASTNode> node) {
        std::string litType = node->getAttribute("type");

        if (litType == "string") {
            uint32_t sidx = constantPool.addString(node->value);
            writer.writeByte(LDC);
            writer.writeInt(sidx);
            pushStack();
            return;
        }

        if (litType == "float") {
            float f = std::stof(node->value);
            uint32_t idx = constantPool.addFloat(f);
            writer.writeByte(LDC);
            writer.writeInt(idx);
            pushStack();
            return;
        }

        if (litType == "bool") {
            int32_t b = (node->value == "true") ? 1 : 0;
            uint32_t idx = constantPool.addInt(b);
            writer.writeByte(LDC);
            writer.writeInt(idx);
            pushStack();
            writer.writeByte(BOOLIFY);
            // stack size unchanged
            return;
        }

        // default: int
        int32_t i = std::stoi(node->value);
        uint32_t idx = constantPool.addInt(i);
        writer.writeByte(LDC);
        writer.writeInt(idx);
        pushStack();
    }


    void generateIdentifier(std::shared_ptr<ASTNode> node) {
        uint16_t localIdx = getOrCreateLocal(node->value);
        writer.writeByte(LOAD);
        writer.writeShort(localIdx);
        pushStack();
    }

    void generateBinaryOp(std::shared_ptr<ASTNode> node) {
        generateExpression(node->children[0]);
        generateExpression(node->children[1]);

        // Arithmetic
        if (node->value == "+") writer.writeByte(ADD);
        else if (node->value == "-") writer.writeByte(SUB);
        else if (node->value == "*") writer.writeByte(MUL);
        else if (node->value == "/") writer.writeByte(DIV);

        // Comparisons (produce a boolean Value)
        else if (node->value == "==") writer.writeByte(ICMP_EQ);
        else if (node->value == "!=") writer.writeByte(ICMP_NE);
        else if (node->value == "<")  writer.writeByte(ICMP_LT);
        else if (node->value == ">")  writer.writeByte(ICMP_GT);
        else if (node->value == "<=") writer.writeByte(ICMP_LE);
        else if (node->value == ">=") writer.writeByte(ICMP_GE);
        else {
            throw std::runtime_error("Operator not implemented in v0.x: " + node->value);
        }

        // All binary ops consume 2 stack values and leave 1 result.
        popStack();
    }


    void generateUnaryOp(std::shared_ptr<ASTNode> node) {
        generateExpression(node->children[0]);
        if (node->value == "-") writer.writeByte(NEG);
    }

    void setUserLibraryFunctions(const std::vector<std::string>& funcNames) {
        userLibraryFunctions = funcNames;
    }

    // Method to compile a user library file and merge its functions
    void compileUserLibrary(const std::string& libPath) {
        // Read the library file
        std::ifstream file(libPath);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open library file: " << libPath << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        // Parse the library (need to include lexer/parser here or pass tokens)
        // For now, we'll handle this in the main compiler flow
    }

    // Update generateFunctionCall to check user library functions
    bool generateFunctionCall(std::shared_ptr<ASTNode> node) {
        std::string funcName = node->value;
        std::string libName = node->getAttribute("library");
        bool isBuiltin = (node->getAttribute("builtin") == "true");

        // Check if this is a builtin library function
        if (!libName.empty() && isBuiltin) {
            return generateBuiltinLibraryCall(libName, funcName, node);
        }

        // Built-in: arg(n) - fetch nth argument of current function call
        if (funcName == "arg" && node->children.size() == 1) {
            auto idxNode = node->children[0];
            if (idxNode->type != NodeType::LITERAL || idxNode->getAttribute("type") != "int") {
                throw std::runtime_error("arg(n) requires an integer literal (v0.x)");
            }
            uint16_t argIdx = static_cast<uint16_t>(std::stoi(idxNode->value));
            writer.writeByte(ARG_GET);
            writer.writeShort(argIdx);
            pushStack();
            return true;
        }

        // Built-in: len(array)
        if (funcName == "len" && node->children.size() == 1) {
            generateExpression(node->children[0]);
            writer.writeByte(ARRAYLENGTH);
            return true;
        }

        // Built-in: write(var)
        if (funcName == "write" && node->children.size() == 1) {
            if (node->children[0]->type != NodeType::IDENTIFIER) {
                throw std::runtime_error("write(...) requires a variable name");
            }
            uint16_t slot = getOrCreateLocal(node->children[0]->value);
            writer.writeByte(WRITE_LOCAL);
            writer.writeShort(slot);
            return false;
        }

        // Built-in privileged dispatcher: exec("op", ...)
        if (funcName == "exec") {
            if (currentFuncType != "Exec") {
                throw std::runtime_error("exec(...) may only be called from func(Exec)");
            }
            if (node->children.empty()) {
                throw std::runtime_error("exec(...) requires at least an operation string");
            }

            auto opNode = node->children[0];
            if (opNode->type != NodeType::LITERAL || opNode->getAttribute("type") != "string") {
                throw std::runtime_error("exec(...) first argument must be a string literal (v0.x)");
            }
            uint32_t opIdx = constantPool.addString(opNode->value);

            uint16_t posArgc = 0;
            uint16_t namedArgc = 0;

            for (size_t i = 1; i < node->children.size(); i++) {
                auto arg = node->children[i];
                if (arg->type == NodeType::NAMED_ARG) {
                    uint32_t nameIdx = constantPool.addString(arg->value);
                    writer.writeByte(LDC);
                    writer.writeInt(nameIdx);
                    pushStack();
                    generateExpression(arg->children[0]);
                    namedArgc++;
                } else {
                    generateExpression(arg);
                    posArgc++;
                }
            }

            writer.writeByte(EXEC);
            writer.writeInt(opIdx);
            writer.writeShort(posArgc);
            writer.writeShort(namedArgc);

            popStack(posArgc + namedArgc * 2);
            pushStack();
            return true;
        }

        // Generate arguments for regular calls
        for (auto& arg : node->children) {
            generateExpression(arg);
        }

        // Check if it's a user library function
        if (!libName.empty() && !isBuiltin) {
            // User library function - should be in funcIndex
            auto it = funcIndex.find(funcName);
            if (it != funcIndex.end()) {
                writer.writeByte(CALL);
                writer.writeInt(it->second);
                writer.writeShort(static_cast<uint16_t>(node->children.size()));
                popStack(static_cast<int>(node->children.size()));
                pushStack();          // return value now on stack
                return true;
            }
        }

        // Regular function call
        auto it = funcIndex.find(funcName);
        if (it == funcIndex.end()) {
            throw std::runtime_error("Unknown function: " + funcName);
        }
        writer.writeByte(CALL);
        writer.writeInt(it->second);
        writer.writeShort(static_cast<uint16_t>(node->children.size()));
        popStack(static_cast<int>(node->children.size()));
        pushStack();          // return value now on stack
        return true;
    }

    void generateArrayAccess(std::shared_ptr<ASTNode> node) {
        // base expression (identifier) already in child[0]
        generateExpression(node->children[0]);
        generateExpression(node->children[1]);
        writer.writeByte(INDEX_GET);
        popStack(1); // two -> one
    }


    void generateArrayLiteral(std::shared_ptr<ASTNode> node) {
        // Allocate array of length N
        writer.writeByte(NEWARRAY);
        writer.writeInt((uint32_t)node->children.size());
        pushStack(); // array ref on stack

        // We'll store it in a temp local so we can fill values without a DUP opcode
        uint16_t tmp = nextLocalIndex++;
        writer.writeByte(STORE);
        writer.writeShort(tmp);
        popStack(); // consumed array ref

        // Fill elements: arr[i] = value
        for (uint32_t i = 0; i < node->children.size(); i++) {
            writer.writeByte(LOAD);
            writer.writeShort(tmp);
            pushStack(); // arr ref

            // index
            uint32_t idxConst = constantPool.addInt((int32_t)i);
            writer.writeByte(LDC);
            writer.writeInt(idxConst);
            pushStack(); // idx

            // value
            generateExpression(node->children[i]); // pushes value

            writer.writeByte(INDEX_SET);
            popStack(3); // arr, idx, val consumed
        }

        // leave array ref on stack as expression result
        writer.writeByte(LOAD);
        writer.writeShort(tmp);
        pushStack();
    }


    void generateIf(std::shared_ptr<ASTNode> node) {
        generateExpression(node->children[0]);
        popStack();

        writer.writeByte(IFEQ);
        uint32_t elseJmp = writer.getCurrentOffset();
        writer.writeInt(0); // patched to else (or end if no else)

        // then-block
        generateBlock(node->children[1]);

        // optional else-block
        if (node->children.size() > 2 && node->children[2] && node->children[2]->type == NodeType::BLOCK) {
            writer.writeByte(GOTO);
            uint32_t endJmp = writer.getCurrentOffset();
            writer.writeInt(0); // patched to end

            // else target
            writer.patchInt(elseJmp, writer.getCurrentOffset());
            generateBlock(node->children[2]);

            // end target
            writer.patchInt(endJmp, writer.getCurrentOffset());
        } else {
            // no else
            writer.patchInt(elseJmp, writer.getCurrentOffset());
        }
    }


    void generateWhile(std::shared_ptr<ASTNode> node) {
        uint32_t start = writer.getCurrentOffset();
        generateExpression(node->children[0]);
        popStack();

        writer.writeByte(IFEQ);
        uint32_t end = writer.getCurrentOffset();
        writer.writeInt(0);

        generateBlock(node->children[1]);
        writer.writeByte(GOTO);
        writer.writeInt(start);
        writer.patchInt(end, writer.getCurrentOffset());
    }

    void generateFor(std::shared_ptr<ASTNode> node) {
        // Generate initialization (idx = 0)
        if (node->children[0]) {
            generateStatement(node->children[0]);
        }

        // Mark the start of the condition check
        uint32_t condStart = writer.getCurrentOffset();

        // Generate condition (idx < len(a))
        if (node->children[1]) {
            generateExpression(node->children[1]);
            popStack();

            // If condition is false (0), jump to end
            writer.writeByte(IFEQ);
            uint32_t endJump = writer.getCurrentOffset();
            writer.writeInt(0);  // Placeholder for end address

            // Generate loop body
            if (node->children[3]) {
                generateBlock(node->children[3]);
            }

            // Generate increment (idx++)
            if (node->children[2]) {
                generateStatement(node->children[2]);
            }

            // Jump back to condition
            writer.writeByte(GOTO);
            writer.writeInt(condStart);

            // Patch the end jump to point here
            writer.patchInt(endJump, writer.getCurrentOffset());
        }
    }

    void generateReturn(std::shared_ptr<ASTNode> node) {
        // Return value: leave it on the stack for RET to return to the caller.
        if (!node->children.empty()) {
            generateExpression(node->children[0]);
            // do NOT POP here
        }
        writer.writeByte(RET);
    }

    void generatePrint(std::shared_ptr<ASTNode> node) {
        if (node->children.empty()) return;
        generateExpression(node->children[0]);
        writer.writeByte(PRINT);
        popStack();
    }

    // Add to bytecodeGen.cpp - complete library bytecode generation

    // Helper function to generate library function call
    void generateLibCall(const std::string& opName,
                         size_t argc,
                         BytecodeWriter& writer,
                         ConstantPool& constantPool,
                         uint16_t& currentStackDepth,
                         uint16_t& maxStackDepth) {
        uint32_t opIdx = constantPool.addString(opName);
        writer.writeByte(EXEC);
        writer.writeInt(opIdx);
        writer.writeShort(static_cast<uint16_t>(argc));
        writer.writeShort(0);  // 0 named args
    }

    // Update generateBuiltinLibraryCall to handle all 4 libraries
    bool generateBuiltinLibraryCall(const std::string& libName,
                                     const std::string& funcName,
                                     std::shared_ptr<ASTNode> node) {

        size_t argc = node->children.size();

        // Generate arguments
        for (auto& arg : node->children) {
            generateExpression(arg);
        }

        std::string opName;
        bool returnsValue = true;

        // =============================================================================
        // _MATH LIBRARY
        // =============================================================================
        if (libName == "_math") {
            // Single-argument functions
            std::vector<std::string> singleArgMath = {
                "sqrt", "abs", "sin", "cos", "tan", "asin", "acos", "atan",
                "sinh", "cosh", "tanh", "exp", "log", "log10", "log2",
                "floor", "ceil", "round", "trunc", "cbrt", "factorial",
                "mean", "median", "stddev", "variance"
            };

            for (const auto& fname : singleArgMath) {
                if (funcName == fname) {
                    if (argc != 1) {
                        throw std::runtime_error(fname + "() requires 1 argument");
                    }
                    generateLibCall("math." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    return true;
                }
            }

            // Two-argument functions
            std::vector<std::string> twoArgMath = {
                "pow", "atan2", "fmod", "min", "max", "hypot", "gcd", "lcm"
            };

            for (const auto& fname : twoArgMath) {
                if (funcName == fname) {
                    if (argc != 2) {
                        throw std::runtime_error(fname + "() requires 2 arguments");
                    }
                    generateLibCall("math." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    popStack(1);  // 2 args -> 1 result
                    return true;
                }
            }

            // Three-argument functions
            if (funcName == "quad") {
                if (argc != 3) throw std::runtime_error("quad() requires 3 arguments");
                generateLibCall("math.quad", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);  // 3 args -> 1 result
                return true;
            }

            // Array-based functions
            if (funcName == "derivative" || funcName == "integral") {
                if (argc != 2) throw std::runtime_error(funcName + "() requires 2 arguments");
                generateLibCall("math." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }
        }

        // =============================================================================
        // _DATASTR LIBRARY
        // =============================================================================
        if (libName == "_datastr") {
            // Stack operations
            if (funcName == "stack_new") {
                generateLibCall("datastr.stack_new", 0, writer, constantPool, currentStackDepth, maxStackDepth);
                pushStack();
                return true;
            }

            if (funcName == "stack_push") {
                if (argc != 2) throw std::runtime_error("stack_push() requires 2 arguments");
                generateLibCall("datastr.stack_push", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return false;  // void
            }

            if (funcName == "stack_pop" || funcName == "stack_peek") {
                if (argc != 1) throw std::runtime_error(funcName + "() requires 1 argument");
                generateLibCall("datastr." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            if (funcName == "stack_empty") {
                if (argc != 1) throw std::runtime_error("stack_empty() requires 1 argument");
                generateLibCall("datastr.stack_empty", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            // Queue operations
            if (funcName == "queue_new") {
                generateLibCall("datastr.queue_new", 0, writer, constantPool, currentStackDepth, maxStackDepth);
                pushStack();
                return true;
            }

            if (funcName == "queue_enqueue") {
                if (argc != 2) throw std::runtime_error("queue_enqueue() requires 2 arguments");
                generateLibCall("datastr.queue_enqueue", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return false;
            }

            if (funcName == "queue_dequeue" || funcName == "queue_front") {
                if (argc != 1) throw std::runtime_error(funcName + "() requires 1 argument");
                generateLibCall("datastr." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            if (funcName == "queue_empty") {
                if (argc != 1) throw std::runtime_error("queue_empty() requires 1 argument");
                generateLibCall("datastr.queue_empty", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            // Heap operations
            if (funcName == "heap_new") {
                generateLibCall("datastr.heap_new", 0, writer, constantPool, currentStackDepth, maxStackDepth);
                pushStack();
                return true;
            }

            if (funcName == "heap_insert") {
                if (argc != 2) throw std::runtime_error("heap_insert() requires 2 arguments");
                generateLibCall("datastr.heap_insert", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return false;
            }

            if (funcName == "heap_extract" || funcName == "heap_peek") {
                if (argc != 1) throw std::runtime_error(funcName + "() requires 1 argument");
                generateLibCall("datastr." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            // Map operations
            if (funcName == "map_new") {
                generateLibCall("datastr.map_new", 0, writer, constantPool, currentStackDepth, maxStackDepth);
                pushStack();
                return true;
            }

            if (funcName == "map_set") {
                if (argc != 3) throw std::runtime_error("map_set() requires 3 arguments");
                generateLibCall("datastr.map_set", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(3);
                return false;
            }

            if (funcName == "map_get") {
                if (argc != 2) throw std::runtime_error("map_get() requires 2 arguments");
                generateLibCall("datastr.map_get", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }

            if (funcName == "map_has") {
                if (argc != 2) throw std::runtime_error("map_has() requires 2 arguments");
                generateLibCall("datastr.map_has", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }

            // Graph operations
            if (funcName == "graph_new") {
                generateLibCall("datastr.graph_new", 0, writer, constantPool, currentStackDepth, maxStackDepth);
                pushStack();
                return true;
            }

            if (funcName == "graph_add_vertex") {
                if (argc != 2) throw std::runtime_error("graph_add_vertex() requires 2 arguments");
                generateLibCall("datastr.graph_add_vertex", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return false;
            }

            if (funcName == "graph_add_edge") {
                if (argc != 4) throw std::runtime_error("graph_add_edge() requires 4 arguments");
                generateLibCall("datastr.graph_add_edge", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(4);
                return false;
            }
        }

        // =============================================================================
        // _ALG LIBRARY
        // =============================================================================
        if (libName == "_alg") {
            // Sorting algorithms (modify in place, void return)
            std::vector<std::string> sortingAlgs = {
                "quicksort", "mergesort", "heapsort", "bubblesort", "insertionsort"
            };

            for (const auto& fname : sortingAlgs) {
                if (funcName == fname) {
                    if (argc != 1) throw std::runtime_error(fname + "() requires 1 argument (array)");
                    generateLibCall("alg." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    popStack(1);
                    return false;  // void
                }
            }

            // Search algorithms (return index)
            if (funcName == "binary_search" || funcName == "linear_search") {
                if (argc != 2) throw std::runtime_error(funcName + "() requires 2 arguments");
                generateLibCall("alg." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }

            // Graph algorithms
            if (funcName == "bfs" || funcName == "dfs") {
                if (argc != 2) throw std::runtime_error(funcName + "() requires 2 arguments");
                generateLibCall("alg." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }

            if (funcName == "dijkstra") {
                if (argc != 3) throw std::runtime_error("dijkstra() requires 3 arguments");
                generateLibCall("alg.dijkstra", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }

            if (funcName == "shuffle") {
                if (argc != 1) throw std::runtime_error("shuffle() requires 1 argument");
                generateLibCall("alg.shuffle", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return false;  // void
            }
        }

        // =============================================================================
        // _ML LIBRARY
        // =============================================================================
        if (libName == "_ml") {
            // Fitting functions (return model)
            if (funcName == "linear_fit") {
                if (argc != 2) throw std::runtime_error("linear_fit() requires 2 arguments");
                generateLibCall("ml.linear_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(1);
                return true;
            }

            if (funcName == "poly_fit") {
                if (argc != 3) throw std::runtime_error("poly_fit() requires 3 arguments");
                generateLibCall("ml.poly_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }

            if (funcName == "logistic_fit") {
                if (argc != 4) throw std::runtime_error("logistic_fit() requires 4 arguments");
                generateLibCall("ml.logistic_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(3);
                return true;
            }

            if (funcName == "knn_fit") {
                if (argc != 3) throw std::runtime_error("knn_fit() requires 3 arguments");
                generateLibCall("ml.knn_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }

            if (funcName == "kmeans_fit") {
                if (argc != 3) throw std::runtime_error("kmeans_fit() requires 3 arguments");
                generateLibCall("ml.kmeans_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }

            if (funcName == "dtree_fit") {
                if (argc != 3) throw std::runtime_error("dtree_fit() requires 3 arguments");
                generateLibCall("ml.dtree_fit", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }

            // Prediction functions
            std::vector<std::string> predictFuncs = {
                "linear_predict", "poly_predict", "logistic_predict",
                "knn_predict", "kmeans_predict", "dtree_predict", "nn_predict"
            };

            for (const auto& fname : predictFuncs) {
                if (funcName == fname) {
                    if (argc != 2) throw std::runtime_error(fname + "() requires 2 arguments");
                    generateLibCall("ml." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    popStack(1);
                    return true;
                }
            }

            // Neural network
            if (funcName == "nn_new") {
                if (argc != 1) throw std::runtime_error("nn_new() requires 1 argument");
                generateLibCall("ml.nn_new", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            if (funcName == "nn_train") {
                if (argc != 5) throw std::runtime_error("nn_train() requires 5 arguments");
                generateLibCall("ml.nn_train", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(5);
                return false;
            }

            // Activation functions
            std::vector<std::string> activations = {"sigmoid", "relu", "tanh_act"};
            for (const auto& fname : activations) {
                if (funcName == fname) {
                    if (argc != 1) throw std::runtime_error(fname + "() requires 1 argument");
                    generateLibCall("ml." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    return true;
                }
            }

            if (funcName == "softmax") {
                if (argc != 1) throw std::runtime_error("softmax() requires 1 argument");
                generateLibCall("ml.softmax", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            // Loss and metric functions
            std::vector<std::string> metrics = {"mse", "cross_entropy", "r2_score", "accuracy", "confusion_matrix"};
            for (const auto& fname : metrics) {
                if (funcName == fname) {
                    if (argc != 2) throw std::runtime_error(fname + "() requires 2 arguments");
                    generateLibCall("ml." + fname, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                    popStack(1);
                    return true;
                }
            }

            // Preprocessing
            if (funcName == "normalize" || funcName == "standardize") {
                if (argc != 1) throw std::runtime_error(funcName + "() requires 1 argument");
                generateLibCall("ml." + funcName, argc, writer, constantPool, currentStackDepth, maxStackDepth);
                return true;
            }

            if (funcName == "train_test_split") {
                if (argc != 3) throw std::runtime_error("train_test_split() requires 3 arguments");
                generateLibCall("ml.train_test_split", argc, writer, constantPool, currentStackDepth, maxStackDepth);
                popStack(2);
                return true;
            }
        }

        throw std::runtime_error("Unknown library function: " + libName + "." + funcName);
    }
};

} // namespace ClangAX

#endif
