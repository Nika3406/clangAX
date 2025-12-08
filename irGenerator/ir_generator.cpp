#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stack>
#include <sstream>
#include <regex>
#include <set>

// LLVM Headers
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

// ============================================
// AST NODE DEFINITIONS (copied from parser)
// ============================================

enum class NodeType {
    PROGRAM, IMPORT_STMT, EXEC_STMT,
    FUNCTION_DECL, CLASS_DECL, OBJECT_SECTION, MEMBER_SECTION,
    BLOCK, ASSIGNMENT, VAR_DECL, VECTOR_DECL,
    IF_STMT, WHILE_STMT, FOR_STMT, RANGE_FOR,
    RETURN_STMT, PRINT_STMT,
    BINARY_OP, UNARY_OP, FUNCTION_CALL, MEMBER_ACCESS,
    ARRAY_ACCESS, ARRAY_LITERAL,
    LITERAL, IDENTIFIER
};

class ASTNode {
public:
    NodeType type;
    string value;
    int line;
    vector<shared_ptr<ASTNode>> children;
    map<string, string> attributes;

    ASTNode(NodeType t, string v = "", int l = 0)
        : type(t), value(v), line(l) {}

    void addChild(shared_ptr<ASTNode> child) {
        if (child) children.push_back(child);
    }

    void setAttribute(const string& key, const string& val) {
        attributes[key] = val;
    }
};

// ============================================
// TOKEN DEFINITIONS (for lexer)
// ============================================

enum class TokenType {
    INTEGER, FLOAT, STRING, CHAR, BOOLEAN, NUL,
    IDENTIFIER, IMPORT, EXEC, FUNC, CLASS, OBJECT, MEMBER,
    FOR, WHILE, IF, ELSE, IN, RANGE, RETURN,
    PRINT, VECTOR, PUSH, POP, SIZE, LEN,
    TRUE, FALSE, NULL_KW,
    PLUS, MINUS, MULT, DIV, MOD, DOT,
    ASSIGN, EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,
    INC, DEC, PLUS_EQ, MINUS_EQ, MULT_EQ, DIV_EQ,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, HASH,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    string value;
    int line;
    int column;
    Token(TokenType t = TokenType::UNKNOWN, string v = "", int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
};

// Forward declarations for Lexer and Parser
class Lexer;
class Parser;

// ============================================
// LLVM IR GENERATOR
// ============================================

class IRGenerator {
private:
    unique_ptr<LLVMContext> context;
    unique_ptr<Module> module;
    unique_ptr<IRBuilder<>> builder;

    // Symbol tables
    map<string, AllocaInst*> namedValues;        // Local variables
    map<string, GlobalVariable*> globalValues;    // Global variables
    map<string, Function*> functions;             // Function registry
    map<string, Type*> structTypes;               // Class/struct types

    bool hasMain = false;

    // Printf function for print support
    Function* printfFunc = nullptr;
    Function* putsFunc = nullptr;

    // Current function context
    Function* currentFunction = nullptr;

    // Loop context (for break/continue)
    struct LoopContext {
        BasicBlock* continueBB;
        BasicBlock* breakBB;
    };
    stack<LoopContext> loopStack;

public:
    IRGenerator(const string& moduleName) {
        context = make_unique<LLVMContext>();
        module = make_unique<Module>(moduleName, *context);
        builder = make_unique<IRBuilder<>>(*context);

        // Declare printf and puts for print support
        declarePrintf();
        declarePuts();
    }

    // ============================================
    // PRINTF/PUTS DECLARATION
    // ============================================

    void declarePrintf() {
        // Declare: i32 @printf(i8*, ...)
        vector<Type*> printfArgs;
        printfArgs.push_back(getPtrType());

        FunctionType* printfType = FunctionType::get(
            getInt32Type(),
            printfArgs,
            true  // varargs
        );

        printfFunc = Function::Create(
            printfType,
            Function::ExternalLinkage,
            "printf",
            module.get()
        );
    }

    void declarePuts() {
        // Declare: i32 @puts(i8*)
        vector<Type*> putsArgs;
        putsArgs.push_back(getPtrType());

        FunctionType* putsType = FunctionType::get(
            getInt32Type(),
            putsArgs,
            false
        );

        putsFunc = Function::Create(
            putsType,
            Function::ExternalLinkage,
            "puts",
            module.get()
        );
    }

    // ============================================
    // STRING CONSTANT CREATION
    // ============================================

    GlobalVariable* createGlobalString(const string& str, const string& name = "") {
        // Create a constant string in global memory
        Constant* strConstant = ConstantDataArray::getString(*context, str, true);

        GlobalVariable* gvar = new GlobalVariable(
            *module,
            strConstant->getType(),
            true,  // isConstant
            GlobalValue::PrivateLinkage,
            strConstant,
            name.empty() ? ".str" : name
        );

        gvar->setAlignment(Align(1));
        return gvar;
    }

    Value* getStringPtr(const string& str) {
        // Create global string and return pointer to it
        GlobalVariable* gvar = createGlobalString(str);

        // Get pointer to first element
        vector<Value*> indices;
        indices.push_back(ConstantInt::get(*context, APInt(32, 0)));
        indices.push_back(ConstantInt::get(*context, APInt(32, 0)));

        return builder->CreateInBoundsGEP(
            gvar->getValueType(),
            gvar,
            indices,
            "str"
        );
    }

    // ============================================
    // MAIN FUNCTION
    // ============================================
    void declareFunction(shared_ptr<ASTNode> node) {
        string funcName = node->value;

        bool isMain = false;
        auto it = node->attributes.find("type");
        if (it != node->attributes.end() && it->second == "Main") {
            funcName = "main";
            isMain = true;
            hasMain = true;
            cout << "  Found Main function, declaring as 'main'" << endl;
        } else {
            cout << "  Declaring function: " << funcName << endl;
        }

        Type* returnType = isMain ? getInt32Type() : getVoidType();
        FunctionType* funcType = FunctionType::get(returnType, {}, false);
        Function* func = Function::Create(
            funcType,
            Function::ExternalLinkage,
            funcName,
            module.get()
        );

        functions[funcName] = func;
    }

    // ============================================
    // TYPE HELPERS
    // ============================================

    Type* getInt32Type() { return Type::getInt32Ty(*context); }
    Type* getInt64Type() { return Type::getInt64Ty(*context); }
    Type* getFloatType() { return Type::getFloatTy(*context); }
    Type* getDoubleType() { return Type::getDoubleTy(*context); }
    Type* getVoidType() { return Type::getVoidTy(*context); }
    Type* getBoolType() { return Type::getInt1Ty(*context); }
    Type* getInt8Type() { return Type::getInt8Ty(*context); }
    PointerType* getPtrType() { return PointerType::get(*context, 0); }

    Type* getTypeFromString(const string& typeStr) {
        if (typeStr == "int" || typeStr == "integer") return getInt32Type();
        if (typeStr == "float") return getFloatType();
        if (typeStr == "double") return getDoubleType();
        if (typeStr == "bool" || typeStr == "boolean") return getBoolType();
        if (typeStr == "void") return getVoidType();
        return getInt32Type(); // Default
    }

    // ============================================
    // CODE GENERATION FROM AST
    // ============================================

    void generateProgram(shared_ptr<ASTNode> ast) {
        if (!ast || ast->type != NodeType::PROGRAM) {
            cerr << "Error: Invalid AST root" << endl;
            return;
        }

        cout << "Generating IR from AST... (debug check)\n";

        // First pass: declare all functions
        for (auto& child : ast->children) {
            if (child->type == NodeType::FUNCTION_DECL) {
                declareFunction(child);
            }
        }

        // Second pass: generate function bodies
        for (auto& child : ast->children) {
            if (child->type == NodeType::FUNCTION_DECL) {
                generateFunction(child);
            } else if (child->type == NodeType::EXEC_STMT) {
                // Handle exec directives (could be used for optimization hints)
                // For now, we'll skip them
            }
        }

        // Check if main function was created
        if (functions.find("main") == functions.end()) {
            cout << "Warning: No main function found, creating empty main..." << endl;

            // Create a simple main that returns 0
            FunctionType* mainType = FunctionType::get(getInt32Type(), {}, false);
            Function* mainFunc = Function::Create(mainType, Function::ExternalLinkage, "main", module.get());

            BasicBlock* entryBB = BasicBlock::Create(*context, "entry", mainFunc);
            builder->SetInsertPoint(entryBB);
            builder->CreateRet(ConstantInt::get(*context, APInt(32, 0, true)));

            functions["main"] = mainFunc;
        }

        cout << "IR generation completed!" << endl;
    }

    void generateFunction(shared_ptr<ASTNode> node) {
        string funcName = node->value;

        // Check if it's Main function
        bool isMain = false;
        if (node->attributes.count("type") && node->attributes["type"] == "Main") {
            funcName = "main";
            isMain = true;
        }

        Function* func = functions[funcName];
        if (!func) {
            cerr << "Error: Function " << funcName << " not declared" << endl;
            return;
        }

        currentFunction = func;

        // Create entry block
        BasicBlock* entryBB = BasicBlock::Create(*context, "entry", func);
        builder->SetInsertPoint(entryBB);

        // Clear local variable table
        namedValues.clear();

        // Generate function body
        if (!node->children.empty() && node->children[0]->type == NodeType::BLOCK) {
            generateBlock(node->children[0]);
        }

        // Add return if not present
        if (!builder->GetInsertBlock()->getTerminator()) {
            if (isMain) {
                builder->CreateRet(ConstantInt::get(*context, APInt(32, 0, true)));
            } else {
                builder->CreateRetVoid();
            }
        }

        currentFunction = nullptr;
    }

    void generateBlock(shared_ptr<ASTNode> node) {
        for (auto& stmt : node->children) {
            generateStatement(stmt);
        }
    }

    void generateStatement(shared_ptr<ASTNode> node) {
        switch (node->type) {
            case NodeType::ASSIGNMENT:
                generateAssignment(node);
                break;
            case NodeType::IF_STMT:
                generateIf(node);
                break;
            case NodeType::WHILE_STMT:
                generateWhile(node);
                break;
            case NodeType::FOR_STMT:
                generateFor(node);
                break;
            case NodeType::RETURN_STMT:
                generateReturn(node);
                break;
            case NodeType::PRINT_STMT:
                generatePrint(node);
                break;
            case NodeType::VECTOR_DECL:
                generateVectorDecl(node);
                break;
            case NodeType::FUNCTION_CALL:
                generateExpression(node); // Function call as statement
                break;
            case NodeType::UNARY_OP:
                generateExpression(node); // Unary op as statement (i++, etc)
                break;
            default:
                // Other statement types
                break;
        }
    }

    void generateAssignment(shared_ptr<ASTNode> node) {
        string varName = node->value;

        if (node->children.empty()) {
            cerr << "Error: Assignment has no value" << endl;
            return;
        }

        Value* value = generateExpression(node->children[0]);
        if (!value) return;

        // Check if variable exists
        AllocaInst* var = namedValues[varName];

        if (!var) {
            // Create new variable with appropriate type
            Type* allocaType = value->getType();

            // Special handling for array literals - allocate array type
            if (node->children[0]->type == NodeType::ARRAY_LITERAL) {
                int arraySize = node->children[0]->children.size();
                if (arraySize > 0) {
                    // Get the type of first element
                    Type* elemType = value->getType();
                    allocaType = ArrayType::get(elemType, arraySize);
                }
            }

            var = createEntryBlockAlloca(currentFunction, varName, allocaType);
            namedValues[varName] = var;
        }

        builder->CreateStore(value, var);
    }

    void generateIf(shared_ptr<ASTNode> node) {
        if (node->children.size() < 2) return;

        Value* cond = generateExpression(node->children[0]);
        if (!cond) return;

        // Convert condition to boolean if needed
        if (cond->getType() != getBoolType()) {
            cond = builder->CreateICmpNE(cond,
                ConstantInt::get(cond->getType(), 0), "ifcond");
        }

        BasicBlock* thenBB = BasicBlock::Create(*context, "then", currentFunction);
        BasicBlock* elseBB = node->children.size() > 2 ?
            BasicBlock::Create(*context, "else") : nullptr;
        BasicBlock* mergeBB = BasicBlock::Create(*context, "ifcont");

        if (elseBB) {
            builder->CreateCondBr(cond, thenBB, elseBB);
        } else {
            builder->CreateCondBr(cond, thenBB, mergeBB);
        }

        // Then block
        builder->SetInsertPoint(thenBB);
        generateBlock(node->children[1]);
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }

        // Else block
        if (elseBB) {
            currentFunction->insert(currentFunction->end(), elseBB);
            builder->SetInsertPoint(elseBB);
            generateBlock(node->children[2]);
            if (!builder->GetInsertBlock()->getTerminator()) {
                builder->CreateBr(mergeBB);
            }
        }

        // Merge block
        currentFunction->insert(currentFunction->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);
    }

    void generateWhile(shared_ptr<ASTNode> node) {
        if (node->children.size() < 2) return;

        BasicBlock* condBB = BasicBlock::Create(*context, "whilecond", currentFunction);
        BasicBlock* bodyBB = BasicBlock::Create(*context, "whilebody");
        BasicBlock* afterBB = BasicBlock::Create(*context, "afterwhile");

        builder->CreateBr(condBB);
        builder->SetInsertPoint(condBB);

        Value* cond = generateExpression(node->children[0]);
        if (!cond) return;

        if (cond->getType() != getBoolType()) {
            cond = builder->CreateICmpNE(cond,
                ConstantInt::get(cond->getType(), 0), "whilecond");
        }

        builder->CreateCondBr(cond, bodyBB, afterBB);

        currentFunction->insert(currentFunction->end(), bodyBB);
        builder->SetInsertPoint(bodyBB);

        // Push loop context
        loopStack.push({condBB, afterBB});

        generateBlock(node->children[1]);

        // Pop loop context
        loopStack.pop();

        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(condBB);
        }

        currentFunction->insert(currentFunction->end(), afterBB);
        builder->SetInsertPoint(afterBB);
    }

    void generateFor(shared_ptr<ASTNode> node) {
        if (node->children.size() < 4) return;

        // Init
        generateStatement(node->children[0]);

        BasicBlock* condBB = BasicBlock::Create(*context, "forcond", currentFunction);
        BasicBlock* bodyBB = BasicBlock::Create(*context, "forbody");
        BasicBlock* incBB = BasicBlock::Create(*context, "forinc");
        BasicBlock* afterBB = BasicBlock::Create(*context, "afterfor");

        builder->CreateBr(condBB);
        builder->SetInsertPoint(condBB);

        Value* cond = generateExpression(node->children[1]);
        if (!cond) return;

        if (cond->getType() != getBoolType()) {
            cond = builder->CreateICmpNE(cond,
                ConstantInt::get(cond->getType(), 0), "forcond");
        }

        builder->CreateCondBr(cond, bodyBB, afterBB);

        currentFunction->insert(currentFunction->end(), bodyBB);
        builder->SetInsertPoint(bodyBB);

        // Push loop context
        loopStack.push({incBB, afterBB});

        generateBlock(node->children[3]);

        // Pop loop context
        loopStack.pop();

        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(incBB);
        }

        currentFunction->insert(currentFunction->end(), incBB);
        builder->SetInsertPoint(incBB);

        generateStatement(node->children[2]); // Increment

        builder->CreateBr(condBB);

        currentFunction->insert(currentFunction->end(), afterBB);
        builder->SetInsertPoint(afterBB);
    }

    void generateReturn(shared_ptr<ASTNode> node) {
        if (node->children.empty()) {
            builder->CreateRetVoid();
        } else {
            Value* retVal = generateExpression(node->children[0]);
            if (retVal) {
                builder->CreateRet(retVal);
            }
        }
    }

    void generatePrint(shared_ptr<ASTNode> node) {
        if (node->children.empty()) return;

        // Get the value to print
        Value* val = generateExpression(node->children[0]);
        if (!val) return;

        Type* valType = val->getType();

        // Create format string based on type
        string formatStr;
        if (valType->isIntegerTy(32)) {
            formatStr = "%d\n";
        } else if (valType->isIntegerTy(8)) {
            formatStr = "%c\n";
        } else if (valType->isIntegerTy(1)) {
            formatStr = "%d\n";  // Print bool as 0/1
        } else if (valType->isDoubleTy() || valType->isFloatTy()) {
            formatStr = "%f\n";
        } else if (valType->isPointerTy()) {
            // Assume it's a string pointer
            formatStr = "%s\n";
        } else {
            // Unknown type, print as integer
            formatStr = "%d\n";
        }

        // Get format string pointer
        Value* formatStrPtr = getStringPtr(formatStr);

        // Call printf
        vector<Value*> printfArgs;
        printfArgs.push_back(formatStrPtr);
        printfArgs.push_back(val);

        builder->CreateCall(printfFunc, printfArgs);
    }

    void generateVectorDecl(shared_ptr<ASTNode> node) {
        string varName = node->value;
        // Allocate space for vector pointer
        // For simplicity, we'll treat vectors as pointers
        AllocaInst* var = createEntryBlockAlloca(currentFunction, varName, getPtrType());
        namedValues[varName] = var;
    }

    Value* generateExpression(shared_ptr<ASTNode> node) {
        switch (node->type) {
            case NodeType::LITERAL:
                return generateLiteral(node);
            case NodeType::IDENTIFIER:
                return generateIdentifier(node);
            case NodeType::BINARY_OP:
                return generateBinaryOp(node);
            case NodeType::UNARY_OP:
                return generateUnaryOp(node);
            case NodeType::FUNCTION_CALL:
                return generateFunctionCall(node);
            case NodeType::ARRAY_ACCESS:
                return generateArrayAccess(node);
            case NodeType::ARRAY_LITERAL:
                return generateArrayLiteral(node);
            default:
                return nullptr;
        }
    }

    Value* generateLiteral(shared_ptr<ASTNode> node) {
        string val = node->value;

        // Check for boolean first
        if (val == "true") {
            return ConstantInt::get(*context, APInt(1, 1));
        }
        if (val == "false") {
            return ConstantInt::get(*context, APInt(1, 0));
        }

        // Check for null
        if (val == "null") {
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        // Check for character literal (single char in value)
        if (val.length() == 1 && !isdigit(val[0]) && val[0] != '-') {
            // This is a char literal
            return ConstantInt::get(*context, APInt(8, (uint8_t)val[0], false));
        }

        // Check if it contains a decimal point (float)
        if (val.find('.') != string::npos) {
            try {
                double floatVal = stod(val);
                return ConstantFP::get(*context, APFloat(floatVal));
            } catch (...) {}
        }

        // Try to parse as integer
        try {
            // Handle negative numbers
            if (val[0] == '-' || isdigit(val[0])) {
                int intVal = stoi(val);
                return ConstantInt::get(*context, APInt(32, intVal, true));
            }
        } catch (...) {}

        // String literals - create global string constant
        if (!val.empty() && !isdigit(val[0]) && val != "true" && val != "false") {
            // This is a string literal
            return getStringPtr(val);
        }

        // Default to 0
        return ConstantInt::get(*context, APInt(32, 0, true));
    }

    Value* generateIdentifier(shared_ptr<ASTNode> node) {
        string name = node->value;

        AllocaInst* var = namedValues[name];
        if (!var) {
            cerr << "Error: Unknown variable: " << name << endl;
            return nullptr;
        }

        return builder->CreateLoad(var->getAllocatedType(), var, name.c_str());
    }

    Value* generateBinaryOp(shared_ptr<ASTNode> node) {
        if (node->children.size() < 2) return nullptr;

        Value* lhs = generateExpression(node->children[0]);
        Value* rhs = generateExpression(node->children[1]);

        if (!lhs || !rhs) return nullptr;

        string op = node->value;

        // Arithmetic operations
        if (op == "+") {
            if (lhs->getType()->isFloatingPointTy()) {
                return builder->CreateFAdd(lhs, rhs, "addtmp");
            }
            return builder->CreateAdd(lhs, rhs, "addtmp");
        }
        if (op == "-") {
            if (lhs->getType()->isFloatingPointTy()) {
                return builder->CreateFSub(lhs, rhs, "subtmp");
            }
            return builder->CreateSub(lhs, rhs, "subtmp");
        }
        if (op == "*") {
            if (lhs->getType()->isFloatingPointTy()) {
                return builder->CreateFMul(lhs, rhs, "multmp");
            }
            return builder->CreateMul(lhs, rhs, "multmp");
        }
        if (op == "/") {
            if (lhs->getType()->isFloatingPointTy()) {
                return builder->CreateFDiv(lhs, rhs, "divtmp");
            }
            return builder->CreateSDiv(lhs, rhs, "divtmp");
        }
        if (op == "%") {
            return builder->CreateSRem(lhs, rhs, "modtmp");
        }

        // Comparison operations
        if (op == "<") {
            return builder->CreateICmpSLT(lhs, rhs, "cmptmp");
        }
        if (op == ">") {
            return builder->CreateICmpSGT(lhs, rhs, "cmptmp");
        }
        if (op == "<=") {
            return builder->CreateICmpSLE(lhs, rhs, "cmptmp");
        }
        if (op == ">=") {
            return builder->CreateICmpSGE(lhs, rhs, "cmptmp");
        }
        if (op == "==") {
            return builder->CreateICmpEQ(lhs, rhs, "cmptmp");
        }
        if (op == "!=") {
            return builder->CreateICmpNE(lhs, rhs, "cmptmp");
        }

        // Logical operations
        if (op == "&&") {
            return builder->CreateAnd(lhs, rhs, "andtmp");
        }
        if (op == "||") {
            return builder->CreateOr(lhs, rhs, "ortmp");
        }

        return nullptr;
    }

    Value* generateUnaryOp(shared_ptr<ASTNode> node) {
        if (node->children.empty()) return nullptr;

        string op = node->value;

        // Handle increment/decrement
        if (op == "++post" || op == "--post" || op == "++" || op == "--") {
            if (node->children[0]->type != NodeType::IDENTIFIER) return nullptr;

            string varName = node->children[0]->value;
            AllocaInst* var = namedValues[varName];
            if (!var) return nullptr;

            Value* val = builder->CreateLoad(var->getAllocatedType(), var, varName.c_str());
            Value* one = ConstantInt::get(val->getType(), 1);

            Value* newVal;
            if (op == "++post" || op == "++") {
                newVal = builder->CreateAdd(val, one, "inc");
            } else {
                newVal = builder->CreateSub(val, one, "dec");
            }

            builder->CreateStore(newVal, var);
            return val; // Return old value for post-increment
        }

        Value* operand = generateExpression(node->children[0]);
        if (!operand) return nullptr;

        if (op == "-") {
            if (operand->getType()->isFloatingPointTy()) {
                return builder->CreateFNeg(operand, "negtmp");
            }
            return builder->CreateNeg(operand, "negtmp");
        }
        if (op == "!") {
            return builder->CreateNot(operand, "nottmp");
        }

        return nullptr;
    }

    Value* generateFunctionCall(shared_ptr<ASTNode> node) {
        string funcName = node->value;

        // Handle special built-in functions
        if (funcName == "len") {
            // For arrays, return their length
            if (!node->children.empty()) {
                // Check if the argument is an identifier that refers to an array
                if (node->children[0]->type == NodeType::IDENTIFIER) {
                    string arrayName = node->children[0]->value;
                    AllocaInst* arrayVar = namedValues[arrayName];

                    if (arrayVar) {
                        Type* allocatedType = arrayVar->getAllocatedType();
                        if (ArrayType* arrayType = dyn_cast<ArrayType>(allocatedType)) {
                            uint64_t arraySize = arrayType->getNumElements();
                            return ConstantInt::get(*context, APInt(32, arraySize, true));
                        }
                    }
                }
            }
            // Fallback: return 0
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        if (funcName == "size") {
            // Similar to len, but for vectors
            // For now, return 0
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        if (funcName == "push" || funcName == "pop") {
            // Vector operations - skip for now
            return nullptr;
        }

        // Regular function call
        Function* func = functions[funcName];
        if (!func) {
            cerr << "Error: Unknown function: " << funcName << endl;
            return nullptr;
        }

        vector<Value*> args;
        for (auto& child : node->children) {
            if (child->type != NodeType::IDENTIFIER ||
                child->value != funcName) { // Skip object reference
                Value* arg = generateExpression(child);
                if (arg) args.push_back(arg);
                }
        }

        // Do NOT give calls to void-valued functions a name
        CallInst* call = builder->CreateCall(func, args);

        if (func->getReturnType()->isVoidTy()) {
            // Statement-only call, nothing to return as a value
            return nullptr;
        } else {
            // Expression with a value (e.g., future non-void user functions)
            return call;
        }
    }

    Value* generateArrayLiteral(shared_ptr<ASTNode> node) {
        if (node->children.empty()) {
            // Empty array - return null pointer
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        // Get all element values
        vector<Constant*> elements;
        Type* elemType = nullptr;
        bool hasMixedTypes = false;

        for (auto& child : node->children) {
            Value* elemVal = generateExpression(child);
            if (!elemVal) continue;

            if (!elemType) {
                elemType = elemVal->getType();
            } else if (elemType != elemVal->getType()) {
                // Mixed types detected
                hasMixedTypes = true;
            }

            if (Constant* constVal = dyn_cast<Constant>(elemVal)) {
                elements.push_back(constVal);
            } else {
                // Non-constant element - can't create constant array
                return ConstantInt::get(*context, APInt(32, 0, true));
            }
        }

        if (elements.empty() || !elemType) {
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        // If mixed types, just use the first element's type and ignore incompatible elements
        // This is a simplified approach - in production you'd want proper type coercion
        if (hasMixedTypes) {
            // Create array with only compatible elements
            vector<Constant*> compatibleElements;

            for (auto* elem : elements) {
                if (elem->getType() == elemType) {
                    compatibleElements.push_back(elem);
                } else {
                    // Skip incompatible types or add a zero/default value
                    if (elemType->isIntegerTy()) {
                        compatibleElements.push_back(
                            ConstantInt::get(elemType, 0)
                        );
                    } else if (elemType->isFloatingPointTy()) {
                        compatibleElements.push_back(
                            ConstantFP::get(elemType, 0.0)
                        );
                    } else if (elemType->isPointerTy()) {
                        compatibleElements.push_back(
                            ConstantPointerNull::get(cast<PointerType>(elemType))
                        );
                    }
                }
            }

            if (compatibleElements.empty()) {
                return ConstantInt::get(*context, APInt(32, 0, true));
            }

            // Create array with compatible elements
            ArrayType* arrayType = ArrayType::get(elemType, compatibleElements.size());
            Constant* arrayConstant = ConstantArray::get(arrayType, compatibleElements);

            AllocaInst* arrayAlloca = builder->CreateAlloca(arrayType, nullptr, "array");
            builder->CreateStore(arrayConstant, arrayAlloca);

            return arrayAlloca;
        }

        // Uniform type array
        ArrayType* arrayType = ArrayType::get(elemType, elements.size());
        Constant* arrayConstant = ConstantArray::get(arrayType, elements);

        // Allocate space for array and store it
        AllocaInst* arrayAlloca = builder->CreateAlloca(arrayType, nullptr, "array");
        builder->CreateStore(arrayConstant, arrayAlloca);

        // Return pointer to array
        return arrayAlloca;
    }

    Value* generateArrayAccess(shared_ptr<ASTNode> node) {
        if (node->children.size() < 2) {
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        // Get array and index
        Value* array = generateExpression(node->children[0]);
        Value* index = generateExpression(node->children[1]);

        if (!array || !index) {
            return ConstantInt::get(*context, APInt(32, 0, true));
        }

        // If array is an alloca (pointer to array), load element
        if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(array)) {
            Type* allocatedType = allocaInst->getAllocatedType();

            if (ArrayType* arrayType = dyn_cast<ArrayType>(allocatedType)) {
                // GEP to get pointer to element
                vector<Value*> indices;
                indices.push_back(ConstantInt::get(*context, APInt(32, 0)));
                indices.push_back(index);

                Value* elemPtr = builder->CreateInBoundsGEP(
                    arrayType,
                    array,
                    indices,
                    "arrayelem"
                );

                // Load the element
                return builder->CreateLoad(arrayType->getElementType(), elemPtr, "arrayval");
            }
        }

        // Fallback: return 0
        return ConstantInt::get(*context, APInt(32, 0, true));
    }

    // ============================================
    // UTILITY FUNCTIONS
    // ============================================

    AllocaInst* createEntryBlockAlloca(Function* func, const string& varName, Type* type) {
        IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
        return tmpBuilder.CreateAlloca(type, nullptr, varName);
    }

    void printIR() {
        module->print(outs(), nullptr);
    }

    void writeIRToFile(const string& filename) {
        error_code ec;
        raw_fd_ostream outFile(filename, ec);

        if (ec) {
            cerr << "Error opening file: " << ec.message() << endl;
            return;
        }

        module->print(outFile, nullptr);
        outFile.close();
        cout << "IR written to: " << filename << endl;
    }

    bool verify() {
        string errorMsg;
        raw_string_ostream errorStream(errorMsg);

        if (verifyModule(*module, &errorStream)) {
            cerr << "Module verification failed:\n" << errorStream.str() << endl;
            return false;
        }

        cout << "Module verification passed!" << endl;
        return true;
    }
};

// ============================================
// LEXER IMPLEMENTATION (from parser.cpp)
// ============================================

class Lexer {
private:
    string source;
    size_t pos;
    int line;
    int column;

    const set<string> keywords = {
        "func", "class", "object", "member", "import", "exec",
        "for", "while", "if", "else", "in", "range", "return",
        "print", "vector", "push", "pop", "size", "len",
        "true", "false", "null"
    };

    char peek(int offset = 0) {
        if (pos + offset >= source.length()) return '\0';
        return source[pos + offset];
    }

    char advance() {
        if (pos >= source.length()) return '\0';
        char c = source[pos++];
        if (c == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
        return c;
    }

    void skipWhitespace() {
        while (isspace(peek())) advance();
    }

    void skipComment() {
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
        }
    }

    Token readNumber() {
        int startLine = line, startCol = column;
        string num;
        bool isFloat = false;

        if (peek() == '-') num += advance();

        while (isdigit(peek()) || peek() == '.') {
            if (peek() == '.') {
                if (isFloat) break;
                isFloat = true;
            }
            num += advance();
        }

        return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER,
                    num, startLine, startCol);
    }

    Token readString() {
        int startLine = line, startCol = column;
        char quote = advance();
        string str;

        while (peek() != quote && peek() != '\0') {
            if (peek() == '\\') {
                advance();
                str += advance();
            } else {
                str += advance();
            }
        }

        if (peek() == quote) advance();
        return Token(TokenType::STRING, str, startLine, startCol);
    }

    Token readChar() {
        int startLine = line, startCol = column;
        advance();
        char c = '\0';
        if (peek() != '\'') c = advance();
        if (peek() == '\'') advance();
        return Token(TokenType::CHAR, string(1, c), startLine, startCol);
    }

    Token readIdentifier() {
        int startLine = line, startCol = column;
        string ident;

        while (isalnum(peek()) || peek() == '_') {
            ident += advance();
        }

        if (keywords.count(ident)) {
            if (ident == "true" || ident == "false")
                return Token(TokenType::BOOLEAN, ident, startLine, startCol);
            if (ident == "null")
                return Token(TokenType::NULL_KW, ident, startLine, startCol);
            if (ident == "import")
                return Token(TokenType::IMPORT, ident, startLine, startCol);
            if (ident == "exec")
                return Token(TokenType::EXEC, ident, startLine, startCol);
            if (ident == "func")
                return Token(TokenType::FUNC, ident, startLine, startCol);
            if (ident == "class")
                return Token(TokenType::CLASS, ident, startLine, startCol);
            if (ident == "object")
                return Token(TokenType::OBJECT, ident, startLine, startCol);
            if (ident == "member")
                return Token(TokenType::MEMBER, ident, startLine, startCol);
            if (ident == "for")
                return Token(TokenType::FOR, ident, startLine, startCol);
            if (ident == "while")
                return Token(TokenType::WHILE, ident, startLine, startCol);
            if (ident == "if")
                return Token(TokenType::IF, ident, startLine, startCol);
            if (ident == "else")
                return Token(TokenType::ELSE, ident, startLine, startCol);
            if (ident == "in")
                return Token(TokenType::IN, ident, startLine, startCol);
            if (ident == "range")
                return Token(TokenType::RANGE, ident, startLine, startCol);
            if (ident == "return")
                return Token(TokenType::RETURN, ident, startLine, startCol);
            if (ident == "print")
                return Token(TokenType::PRINT, ident, startLine, startCol);
            if (ident == "vector")
                return Token(TokenType::VECTOR, ident, startLine, startCol);
        }

        return Token(TokenType::IDENTIFIER, ident, startLine, startCol);
    }

    Token readOperator() {
        int startLine = line, startCol = column;
        char c = peek();

        if (c == '=' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::EQ, "==", startLine, startCol);
        }
        if (c == '!' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::NEQ, "!=", startLine, startCol);
        }
        if (c == '<' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::LTE, "<=", startLine, startCol);
        }
        if (c == '>' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::GTE, ">=", startLine, startCol);
        }
        if (c == '&' && peek(1) == '&') {
            advance(); advance();
            return Token(TokenType::AND, "&&", startLine, startCol);
        }
        if (c == '|' && peek(1) == '|') {
            advance(); advance();
            return Token(TokenType::OR, "||", startLine, startCol);
        }
        if (c == '+' && peek(1) == '+') {
            advance(); advance();
            return Token(TokenType::INC, "++", startLine, startCol);
        }
        if (c == '-' && peek(1) == '-') {
            advance(); advance();
            return Token(TokenType::DEC, "--", startLine, startCol);
        }
        if (c == '+' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::PLUS_EQ, "+=", startLine, startCol);
        }
        if (c == '-' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::MINUS_EQ, "-=", startLine, startCol);
        }
        if (c == '*' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::MULT_EQ, "*=", startLine, startCol);
        }
        if (c == '/' && peek(1) == '=') {
            advance(); advance();
            return Token(TokenType::DIV_EQ, "/=", startLine, startCol);
        }

        advance();
        switch (c) {
            case '+': return Token(TokenType::PLUS, "+", startLine, startCol);
            case '-': return Token(TokenType::MINUS, "-", startLine, startCol);
            case '*': return Token(TokenType::MULT, "*", startLine, startCol);
            case '/': return Token(TokenType::DIV, "/", startLine, startCol);
            case '%': return Token(TokenType::MOD, "%", startLine, startCol);
            case '.': return Token(TokenType::DOT, ".", startLine, startCol);
            case '=': return Token(TokenType::ASSIGN, "=", startLine, startCol);
            case '<': return Token(TokenType::LT, "<", startLine, startCol);
            case '>': return Token(TokenType::GT, ">", startLine, startCol);
            case '!': return Token(TokenType::NOT, "!", startLine, startCol);
            case '(': return Token(TokenType::LPAREN, "(", startLine, startCol);
            case ')': return Token(TokenType::RPAREN, ")", startLine, startCol);
            case '{': return Token(TokenType::LBRACE, "{", startLine, startCol);
            case '}': return Token(TokenType::RBRACE, "}", startLine, startCol);
            case '[': return Token(TokenType::LBRACKET, "[", startLine, startCol);
            case ']': return Token(TokenType::RBRACKET, "]", startLine, startCol);
            case ',': return Token(TokenType::COMMA, ",", startLine, startCol);
            case ':': return Token(TokenType::COLON, ":", startLine, startCol);
            case ';': return Token(TokenType::SEMICOLON, ";", startLine, startCol);
            case '#': return Token(TokenType::HASH, "#", startLine, startCol);
            default: return Token(TokenType::UNKNOWN, string(1, c), startLine, startCol);
        }
    }

public:
    Lexer(const string& src) : source(src), pos(0), line(1), column(0) {}

    vector<Token> tokenize() {
        vector<Token> tokens;

        while (pos < source.length()) {
            skipWhitespace();
            if (pos >= source.length()) break;

            skipComment();
            skipWhitespace();
            if (pos >= source.length()) break;

            char c = peek();

            if (c == '#') {
                tokens.push_back(readOperator());
            } else if (c == '"') {
                tokens.push_back(readString());
            } else if (c == '\'') {
                tokens.push_back(readChar());
            } else if (isdigit(c) || (c == '-' && isdigit(peek(1)))) {
                tokens.push_back(readNumber());
            } else if (isalpha(c) || c == '_') {
                tokens.push_back(readIdentifier());
            } else {
                tokens.push_back(readOperator());
            }
        }

        tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
        return tokens;
    }
};

// ============================================
// PARSER IMPLEMENTATION (simplified from parser.cpp)
// ============================================

class Parser {
private:
    vector<Token> tokens;
    size_t current;
    vector<string> errors;

    Token peek(int offset = 0) {
        if (current + offset >= tokens.size())
            return tokens.back();
        return tokens[current + offset];
    }

    Token advance() {
        Token t = tokens[current];
        if (current < tokens.size() - 1) current++;
        return t;
    }

    bool match(TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(TokenType type, const string& message) {
        if (peek().type != type) {
            errors.push_back("Line " + to_string(peek().line) + ": " + message);
            return Token(TokenType::UNKNOWN, "", peek().line, peek().column);
        }
        return advance();
    }

    shared_ptr<ASTNode> parseProgram();
    shared_ptr<ASTNode> parseFunction();
    shared_ptr<ASTNode> parseBlock();
    shared_ptr<ASTNode> parseStatement();
    shared_ptr<ASTNode> parseAssignment();
    shared_ptr<ASTNode> parseFor();
    shared_ptr<ASTNode> parseWhile();
    shared_ptr<ASTNode> parseIf();
    shared_ptr<ASTNode> parseReturn();
    shared_ptr<ASTNode> parsePrint();
    shared_ptr<ASTNode> parseVectorDecl();
    shared_ptr<ASTNode> parseExpression();
    shared_ptr<ASTNode> parseLogicalOr();
    shared_ptr<ASTNode> parseLogicalAnd();
    shared_ptr<ASTNode> parseEquality();
    shared_ptr<ASTNode> parseComparison();
    shared_ptr<ASTNode> parseTerm();
    shared_ptr<ASTNode> parseFactor();
    shared_ptr<ASTNode> parseUnary();
    shared_ptr<ASTNode> parsePostfix();
    shared_ptr<ASTNode> parsePrimary();

public:
    Parser(const vector<Token>& toks) : tokens(toks), current(0) {}

    shared_ptr<ASTNode> parse() {
        return parseProgram();
    }

    vector<string> getErrors() const {
        return errors;
    }
};

// Parser implementation details... (truncated for brevity - include full parser methods)

shared_ptr<ASTNode> Parser::parseProgram() {
    auto program = make_shared<ASTNode>(NodeType::PROGRAM, "program");

    while (peek().type != TokenType::END_OF_FILE) {
        if (peek().type == TokenType::HASH) {
            advance();
            // Skip imports for now
            if (peek().type == TokenType::IMPORT) {
                advance();
                advance(); // skip string
            }
        } else if (match(TokenType::EXEC)) {
            // Skip exec for now
            advance(); // (
            while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
                advance();
            }
            advance(); // )
        } else if (peek().type == TokenType::FUNC) {
            program->addChild(parseFunction());
        } else if (peek().type == TokenType::CLASS) {
            // Skip a single class definition for now

            advance(); // consume 'class'

            // Skip until we hit the opening '{' of the class body
            while (peek().type != TokenType::LBRACE &&
                   peek().type != TokenType::END_OF_FILE) {
                advance();
                   }

            // Now skip the entire '{ ... }' block, including nested braces
            if (peek().type == TokenType::LBRACE) {
                int depth = 1;
                advance(); // consume '{'

                while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                    if (peek().type == TokenType::LBRACE) {
                        depth++;
                    } else if (peek().type == TokenType::RBRACE) {
                        depth--;
                    }
                    advance();
                }
            }

            // Done skipping ONE class; control returns to the main while loop.
        } else {
            advance();
        }
    }

    return program;
}

shared_ptr<ASTNode> Parser::parseFunction() {
    Token funcToken = expect(TokenType::FUNC, "Expected 'func'");
    expect(TokenType::LPAREN, "Expected '(' after func");

    string funcType = "";
    if (peek().type == TokenType::IDENTIFIER) {
        funcType = advance().value;
    }

    expect(TokenType::RPAREN, "Expected ')' after func type");

    string funcName = "";
    if (peek().type == TokenType::ASSIGN) {
        advance();
        Token nameToken = advance();
        funcName = nameToken.value;
        if ((funcName.front() == '\'' && funcName.back() == '\'') ||
            (funcName.front() == '"' && funcName.back() == '"')) {
            funcName = funcName.substr(1, funcName.length() - 2);
            }
    }

    // Enforce your language rule: Main must be nameless
    if (funcType == "Main" && !funcName.empty()) {
        throw runtime_error(
            "Main function cannot have a name. Use 'func(Main) { ... }' with no '= \"name\"'."
        );
    }

    auto node = make_shared<ASTNode>(
        NodeType::FUNCTION_DECL,
        funcName.empty() ? funcType : funcName,
        funcToken.line
    );
    if (!funcType.empty()) {
        node->setAttribute("type", funcType);
    }

    auto body = parseBlock();
    node->addChild(body);

    return node;
}



shared_ptr<ASTNode> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = make_shared<ASTNode>(NodeType::BLOCK, "block");

    while (peek().type != TokenType::RBRACE && peek().type != TokenType::END_OF_FILE) {
        auto stmt = parseStatement();
        if (stmt) block->addChild(stmt);
    }

    expect(TokenType::RBRACE, "Expected '}'");
    return block;
}

shared_ptr<ASTNode> Parser::parseStatement() {
    if (peek().type == TokenType::FOR) {
        return parseFor();
    } else if (peek().type == TokenType::WHILE) {
        return parseWhile();
    } else if (peek().type == TokenType::IF) {
        return parseIf();
    } else if (peek().type == TokenType::RETURN) {
        return parseReturn();
    } else if (peek().type == TokenType::PRINT) {
        return parsePrint();
    } else if (peek().type == TokenType::VECTOR) {
        return parseVectorDecl();
    } else if (peek().type == TokenType::IDENTIFIER) {
        if (peek(1).type == TokenType::ASSIGN) {
            return parseAssignment();
        }
        return parseExpression();
    }

    return parseExpression();
}

shared_ptr<ASTNode> Parser::parseAssignment() {
    Token var = expect(TokenType::IDENTIFIER, "Expected identifier");
    expect(TokenType::ASSIGN, "Expected '='");
    auto node = make_shared<ASTNode>(NodeType::ASSIGNMENT, var.value);
    node->addChild(parseExpression());
    return node;
}

shared_ptr<ASTNode> Parser::parseFor() {
    Token forToken = expect(TokenType::FOR, "Expected 'for'");
    expect(TokenType::LPAREN, "Expected '(' after for");

    auto node = make_shared<ASTNode>(NodeType::FOR_STMT, "for", forToken.line);

    // Check for range-based for loop: for (x in range(...))
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::IN) {
        Token var = advance();
        expect(TokenType::IN, "Expected 'in'");

        auto rangeNode = make_shared<ASTNode>(NodeType::RANGE_FOR, var.value);
        rangeNode->addChild(parseExpression());
        node->addChild(rangeNode);

        expect(TokenType::RPAREN, "Expected ')' after for");
        node->addChild(parseBlock());
        return node;
    }

    // Traditional for loop: for (init, condition, increment)
    // Parse init (could be assignment or expression)
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::ASSIGN) {
        node->addChild(parseAssignment());
    } else {
        node->addChild(parseExpression());
    }

    expect(TokenType::COMMA, "Expected ',' in for");
    node->addChild(parseExpression()); // condition
    expect(TokenType::COMMA, "Expected ',' in for");
    node->addChild(parseExpression()); // increment
    expect(TokenType::RPAREN, "Expected ')' after for");

    node->addChild(parseBlock());
    return node;
}

shared_ptr<ASTNode> Parser::parseWhile() {
    Token whileToken = expect(TokenType::WHILE, "Expected 'while'");
    expect(TokenType::LPAREN, "Expected '('");
    auto node = make_shared<ASTNode>(NodeType::WHILE_STMT, "while", whileToken.line);
    node->addChild(parseExpression());
    expect(TokenType::RPAREN, "Expected ')'");
    node->addChild(parseBlock());
    return node;
}

shared_ptr<ASTNode> Parser::parseIf() {
    Token ifToken = expect(TokenType::IF, "Expected 'if'");
    expect(TokenType::LPAREN, "Expected '('");
    auto node = make_shared<ASTNode>(NodeType::IF_STMT, "if", ifToken.line);
    node->addChild(parseExpression());
    expect(TokenType::RPAREN, "Expected ')'");
    node->addChild(parseBlock());
    if (match(TokenType::ELSE)) {
        node->addChild(parseBlock());
    }
    return node;
}

shared_ptr<ASTNode> Parser::parseReturn() {
    Token retToken = expect(TokenType::RETURN, "Expected 'return'");
    auto node = make_shared<ASTNode>(NodeType::RETURN_STMT, "return", retToken.line);
    if (peek().type != TokenType::RBRACE) {
        node->addChild(parseExpression());
    }
    return node;
}

shared_ptr<ASTNode> Parser::parsePrint() {
    Token printToken = expect(TokenType::PRINT, "Expected 'print'");
    expect(TokenType::LPAREN, "Expected '('");
    auto node = make_shared<ASTNode>(NodeType::PRINT_STMT, "print", printToken.line);
    if (peek().type != TokenType::RPAREN) {
        node->addChild(parseExpression());
    }
    expect(TokenType::RPAREN, "Expected ')'");
    return node;
}

shared_ptr<ASTNode> Parser::parseVectorDecl() {
    expect(TokenType::VECTOR, "Expected 'vector'");
    expect(TokenType::LT, "Expected '<'");
    Token type = expect(TokenType::IDENTIFIER, "Expected type");
    expect(TokenType::GT, "Expected '>'");
    Token name = expect(TokenType::IDENTIFIER, "Expected identifier");
    auto node = make_shared<ASTNode>(NodeType::VECTOR_DECL, name.value);
    node->setAttribute("elementType", type.value);
    return node;
}

shared_ptr<ASTNode> Parser::parseExpression() { return parseLogicalOr(); }
shared_ptr<ASTNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenType::OR)) {
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, "||");
        node->addChild(left);
        node->addChild(parseLogicalAnd());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseLogicalAnd() {
    auto left = parseEquality();
    while (match(TokenType::AND)) {
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, "&&");
        node->addChild(left);
        node->addChild(parseEquality());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseEquality() {
    auto left = parseComparison();
    while (peek().type == TokenType::EQ || peek().type == TokenType::NEQ) {
        Token op = advance();
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, op.value);
        node->addChild(left);
        node->addChild(parseComparison());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseComparison() {
    auto left = parseTerm();
    while (peek().type == TokenType::LT || peek().type == TokenType::GT ||
           peek().type == TokenType::LTE || peek().type == TokenType::GTE) {
        Token op = advance();
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, op.value);
        node->addChild(left);
        node->addChild(parseTerm());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseTerm() {
    auto left = parseFactor();
    while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
        Token op = advance();
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, op.value);
        node->addChild(left);
        node->addChild(parseFactor());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseFactor() {
    auto left = parseUnary();
    while (peek().type == TokenType::MULT || peek().type == TokenType::DIV ||
           peek().type == TokenType::MOD) {
        Token op = advance();
        auto node = make_shared<ASTNode>(NodeType::BINARY_OP, op.value);
        node->addChild(left);
        node->addChild(parseUnary());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseUnary() {
    if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS ||
        peek().type == TokenType::INC || peek().type == TokenType::DEC) {
        Token op = advance();
        auto node = make_shared<ASTNode>(NodeType::UNARY_OP, op.value);
        node->addChild(parseUnary());
        return node;
    }
    return parsePostfix();
}

shared_ptr<ASTNode> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::INC)) {
            auto node = make_shared<ASTNode>(NodeType::UNARY_OP, "++post");
            node->addChild(expr);
            expr = node;
        } else if (match(TokenType::DEC)) {
            auto node = make_shared<ASTNode>(NodeType::UNARY_OP, "--post");
            node->addChild(expr);
            expr = node;
        } else if (match(TokenType::DOT)) {
            // Member access like vec.size() or vec.push(x)
            Token member = expect(TokenType::IDENTIFIER, "Expected member name");

            if (peek().type == TokenType::LPAREN) {
                // Method call
                advance(); // (
                auto callNode = make_shared<ASTNode>(NodeType::FUNCTION_CALL, member.value);
                callNode->addChild(expr); // Add object as first child

                while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
                    callNode->addChild(parseExpression());
                    if (peek().type == TokenType::COMMA) advance();
                }

                expect(TokenType::RPAREN, "Expected ')' after method call");
                expr = callNode;
            } else {
                // Member access
                auto node = make_shared<ASTNode>(NodeType::MEMBER_ACCESS, member.value);
                node->addChild(expr);
                expr = node;
            }
        } else if (match(TokenType::LBRACKET)) {
            // Array access
            auto node = make_shared<ASTNode>(NodeType::ARRAY_ACCESS, "[]");
            node->addChild(expr);
            node->addChild(parseExpression());
            expect(TokenType::RBRACKET, "Expected ']'");
            expr = node;
        } else if (peek().type == TokenType::LPAREN && expr->type == NodeType::IDENTIFIER) {
            // Function call
            advance(); // (
            auto node = make_shared<ASTNode>(NodeType::FUNCTION_CALL, expr->value);

            while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
                node->addChild(parseExpression());
                if (peek().type == TokenType::COMMA) advance();
            }

            expect(TokenType::RPAREN, "Expected ')' after function call");
            expr = node;
        } else {
            break;
        }
    }

    return expr;
}

shared_ptr<ASTNode> Parser::parsePrimary() {
    if (peek().type == TokenType::INTEGER || peek().type == TokenType::FLOAT ||
        peek().type == TokenType::STRING || peek().type == TokenType::CHAR ||
        peek().type == TokenType::BOOLEAN || peek().type == TokenType::NULL_KW) {
        Token lit = advance();
        return make_shared<ASTNode>(NodeType::LITERAL, lit.value, lit.line);
    }

    if (peek().type == TokenType::IDENTIFIER) {
        Token id = advance();
        return make_shared<ASTNode>(NodeType::IDENTIFIER, id.value, id.line);
    }

    if (match(TokenType::LBRACKET)) {
        // Array literal
        auto node = make_shared<ASTNode>(NodeType::ARRAY_LITERAL, "array");

        while (peek().type != TokenType::RBRACKET && peek().type != TokenType::END_OF_FILE) {
            node->addChild(parseExpression());
            if (peek().type == TokenType::COMMA) advance();
        }

        expect(TokenType::RBRACKET, "Expected ']'");
        return node;
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    // Handle special functions like range(), len(), size()
    if (peek().type == TokenType::RANGE || peek().type == TokenType::LEN ||
        peek().type == TokenType::SIZE) {
        Token func = advance();
        auto node = make_shared<ASTNode>(NodeType::FUNCTION_CALL, func.value);

        expect(TokenType::LPAREN, "Expected '(' after " + func.value);

        while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
            node->addChild(parseExpression());
            if (peek().type == TokenType::COMMA) advance();
        }

        expect(TokenType::RPAREN, "Expected ')'");
        return node;
    }

    advance();
    return make_shared<ASTNode>(NodeType::LITERAL, "0");
}

// ============================================
// MAIN
// ============================================

int main(int argc, char** argv) {
    // Default filename or get from command line
    string filename = "SampleCode.cax";

    if (argc > 1) {
        filename = argv[1];
    }

    cout << "C-ACCEL to LLVM IR Compiler\n";
    cout << "============================\n\n";

    cout << "Reading file: " << filename << "\n\n";

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file: " << filename << "\n";
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    string source = buffer.str();

    cout << "Tokenizing source code...\n";
    Lexer lexer(source);
    vector<Token> tokens = lexer.tokenize();
    cout << "Generated " << tokens.size() << " tokens\n\n";

    cout << "Parsing tokens into AST...\n";
    Parser parser(tokens);
    shared_ptr<ASTNode> ast = parser.parse();

    vector<string> errors = parser.getErrors();
    if (!errors.empty()) {
        cout << "\nPARSE ERRORS DETECTED:\n";
        for (const auto& error : errors) {
            cout << error << "\n";
        }
        return 1;
    }

    cout << "Parsing completed successfully!\n\n";

    // Generate LLVM IR
    IRGenerator gen("C-ACCEL-Module");
    gen.generateProgram(ast);

    cout << "\n" << string(60, '=') << "\n";
    cout << "Generated LLVM IR:\n";
    cout << string(60, '=') << "\n\n";

    gen.printIR();

    cout << "\n" << string(60, '=') << "\n";
    gen.verify();

    string outputFile = "irGenerator/output.ll";
    gen.writeIRToFile(outputFile);

    cout << "\nCompilation completed successfully!\n";
    return 0;
}