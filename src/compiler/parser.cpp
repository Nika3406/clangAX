#ifndef CLANGAX_PARSER_H
#define CLANGAX_PARSER_H

#include "../common/ast.h"
#include "../common/library.h"
#include "lexer.cpp"
#include <iostream>
#include <stdexcept>

namespace ClangAX {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    LibraryManager libraryManager;
    std::map<std::string, std::shared_ptr<Library>> importedLibraries;

    Token currentToken() const {
        return (current < tokens.size()) ? tokens[current] : tokens.back();
    }

    Token peek(int offset = 1) const {
        size_t idx = current + offset;
        return (idx < tokens.size()) ? tokens[idx] : tokens.back();
    }

    void advance() {
        if (current < tokens.size()) {
            current++;
        }
    }

    bool match(TokenType type) {
        if (currentToken().type == type) {
            advance();
            return true;
        }
        return false;
    }

    void expect(TokenType type, const std::string& message) {
        if (!match(type)) {
            throw std::runtime_error("Parse error: " + message +
                                   " at line " + std::to_string(currentToken().line));
        }
    }

    // Parse import directive: #import "library_name"
    void parseImport() {
        expect(TokenType::HASH, "Expected '#'");
        expect(TokenType::IMPORT, "Expected 'import' after '#'");

        if (currentToken().type != TokenType::STRING) {
            throw std::runtime_error("Expected library name as string after #import at line " +
                                   std::to_string(currentToken().line));
        }

        std::string libName = currentToken().value;
        advance();

        // Import the library
        auto lib = libraryManager.importLibrary(libName);
        if (!lib) {
            throw std::runtime_error("Failed to import library: " + libName);
        }

        importedLibraries[libName] = lib;
        std::cout << "Imported library: " << libName << std::endl;
    }

    // Parse program (top level)
    std::shared_ptr<ASTNode> parseProgram() {
        auto program = makeNode(NodeType::PROGRAM, "program");

        // Parse imports first
        while (currentToken().type == TokenType::HASH) {
            parseImport();
        }

        // Store imported libraries in program node for codegen
        for (const auto& [name, lib] : importedLibraries) {
            program->setAttribute("import:" + name, "true");
        }

        // Parse functions
        while (currentToken().type != TokenType::END_OF_FILE) {
            if (currentToken().type == TokenType::FUNC) {
                program->addChild(parseFunction());
            } else {
                throw std::runtime_error("Expected function declaration at line " +
                                       std::to_string(currentToken().line));
            }
        }

        return program;
    }

    // Parse function declaration
    std::shared_ptr<ASTNode> parseFunction() {
        expect(TokenType::FUNC, "Expected 'func'");
        expect(TokenType::LPAREN, "Expected '(' after 'func'");

        // C-Accel syntax: func() = "name" OR func(Type) = "name"
        std::string funcType = "";
        std::string funcName = "";

        // Check if there's a type inside parentheses: func(ML), func(Math), func(Neuro)
        if (currentToken().type == TokenType::IDENTIFIER) {
            funcType = currentToken().value;
            advance();
        }

        expect(TokenType::RPAREN, "Expected ')' after function type");

        // Optional: = "name"
        if (currentToken().type == TokenType::ASSIGN) {
            advance();
            if (currentToken().type == TokenType::STRING) {
                funcName = currentToken().value;
                advance();
            } else {
                throw std::runtime_error("Expected function name as string after '=' at line " +
                                       std::to_string(currentToken().line));
            }
        } else {
            // No name provided, generate one
            // If user wrote func(Main) { ... } treat it as Main
            if (funcType == "Main") {
                funcName = "Main";
            } else {
                funcName = "anonymous_func";
            }
        }

        auto funcNode = makeNode(NodeType::FUNCTION_DECL, funcName);

        // Store function type if provided (ML, Math, Neuro, etc.)
        if (!funcType.empty()) {
            funcNode->setAttribute("func_type", funcType);
        }

        // Check if it's Main function
        if (funcName == "Main") {
            funcNode->setAttribute("type", "Main");
        }

        // Parse function body
        funcNode->addChild(parseBlock());

        return funcNode;
    }

    // Parse block { ... }
    std::shared_ptr<ASTNode> parseBlock() {
        expect(TokenType::LBRACE, "Expected '{'");

        auto block = makeNode(NodeType::BLOCK, "block");

        while (currentToken().type != TokenType::RBRACE &&
               currentToken().type != TokenType::END_OF_FILE) {
            block->addChild(parseStatement());
        }

        expect(TokenType::RBRACE, "Expected '}'");

        return block;
    }

    // Parse statement
    std::shared_ptr<ASTNode> parseStatement() {
        TokenType type = currentToken().type;
        if (type == TokenType::FUNC) {
            return parseFunction();
        }

        if (type == TokenType::IF) {
            return parseIfStatement();
        } else if (type == TokenType::WHILE) {
            return parseWhileStatement();
        } else if (type == TokenType::FOR) {
            return parseForStatement();
        } else if (type == TokenType::RETURN) {
            return parseReturnStatement();
        } else if (type == TokenType::PRINT) {
            return parsePrintStatement();
        } else if (type == TokenType::WRITE) {
            return parseWriteStatement();
        } else if (type == TokenType::LBRACE) {
            return parseBlock();
        } else if (type == TokenType::IDENTIFIER) {
            // Look ahead to determine what kind of statement
            if (peek().type == TokenType::ASSIGN) {
                auto a = parseAssignment();
                match(TokenType::SEMICOLON);
                return a;
            } else if (peek().type == TokenType::LPAREN) {
                // Function call
                auto call = parseFunctionCall();
                match(TokenType::SEMICOLON);
                return call;
            } else if (peek().type == TokenType::INCREMENT || peek().type == TokenType::DECREMENT) {
                // Post-increment/decrement
                std::string varName = currentToken().value;
                advance();
                bool isIncrement = currentToken().type == TokenType::INCREMENT;
                advance();

                auto assignment = makeNode(NodeType::ASSIGNMENT, varName);
                auto binOp = makeNode(NodeType::BINARY_OP, isIncrement ? "+" : "-");
                auto ident = makeNode(NodeType::IDENTIFIER, varName);
                auto one = makeNode(NodeType::LITERAL, "1");
                binOp->addChild(ident);
                binOp->addChild(one);
                assignment->addChild(binOp);

                match(TokenType::SEMICOLON);
                return assignment;
            } else {
                auto expr = parseExpression();
                match(TokenType::SEMICOLON);
                return expr;
            }
        } else {
            throw std::runtime_error(
                "Unexpected token '" + currentToken().value + "' at line " +
                std::to_string(currentToken().line) + ", column " +
                std::to_string(currentToken().column)
            );
        }
    }

    // Parse assignment: identifier = expression
    std::shared_ptr<ASTNode> parseAssignment() {
        if (currentToken().type == TokenType::IDENTIFIER &&
            peek(1).type == TokenType::INCREMENT) {
            std::string varName = currentToken().value;
            advance(); // consume identifier
            advance(); // consume '++'
            return makeNode(NodeType::POST_INCREMENT, varName);
        }

        std::string varName = currentToken().value;
        advance();

        expect(TokenType::ASSIGN, "Expected '='");

        auto assignment = makeNode(NodeType::ASSIGNMENT, varName);
        assignment->addChild(parseExpression());

        return assignment;
    }

    // Parse if statement
    std::shared_ptr<ASTNode> parseIfStatement() {
        expect(TokenType::IF, "Expected 'if'");
        expect(TokenType::LPAREN, "Expected '(' after 'if'");

        auto ifStmt = makeNode(NodeType::IF_STMT, "if");

        // Condition
        ifStmt->addChild(parseExpression());

        expect(TokenType::RPAREN, "Expected ')' after condition");

        // Then block
        ifStmt->addChild(parseBlock());

        // Optional else
        if (match(TokenType::ELSE)) {
            ifStmt->addChild(parseBlock());
        }

        return ifStmt;
    }

    // Parse while statement
    std::shared_ptr<ASTNode> parseWhileStatement() {
        expect(TokenType::WHILE, "Expected 'while'");
        expect(TokenType::LPAREN, "Expected '(' after 'while'");

        auto whileStmt = makeNode(NodeType::WHILE_STMT, "while");

        // Condition
        whileStmt->addChild(parseExpression());

        expect(TokenType::RPAREN, "Expected ')' after condition");

        // Body
        whileStmt->addChild(parseBlock());

        return whileStmt;
    }

    std::shared_ptr<ASTNode> parseForStatement() {
        expect(TokenType::FOR, "Expected 'for'");
        expect(TokenType::LPAREN, "Expected '(' after 'for'");

        auto forStmt = makeNode(NodeType::FOR_STMT, "for");

        // Init (optional)
        if (currentToken().type != TokenType::SEMICOLON) {
            forStmt->addChild(parseAssignment());
        } else {
            forStmt->addChild(nullptr);
        }
        expect(TokenType::SEMICOLON, "Expected ';' after init");

        // Condition (optional)
        if (currentToken().type != TokenType::SEMICOLON) {
            forStmt->addChild(parseExpression());
        } else {
            forStmt->addChild(nullptr);
        }
        expect(TokenType::SEMICOLON, "Expected ';' after condition");

        // Increment (optional)
        if (currentToken().type != TokenType::RPAREN) {
            forStmt->addChild(parseAssignment());
        } else {
            forStmt->addChild(nullptr);
        }

        expect(TokenType::RPAREN, "Expected ')' after increment");

        forStmt->addChild(parseBlock());
        return forStmt;
    }

    // Parse return statement
    std::shared_ptr<ASTNode> parseReturnStatement() {
        expect(TokenType::RETURN, "Expected 'return'");

        auto returnStmt = makeNode(NodeType::RETURN_STMT, "return");

        // Optional return value
        if (currentToken().type != TokenType::SEMICOLON &&
            currentToken().type != TokenType::RBRACE) {
            returnStmt->addChild(parseExpression());
        }

        return returnStmt;
    }

    // Parse print statement
    std::shared_ptr<ASTNode> parsePrintStatement() {
        expect(TokenType::PRINT, "Expected 'print'");
        expect(TokenType::LPAREN, "Expected '(' after 'print'");

        auto printStmt = makeNode(NodeType::PRINT_STMT, "print");

        printStmt->addChild(parseExpression());

        expect(TokenType::RPAREN, "Expected ')' after expression");

        match(TokenType::SEMICOLON);

        return printStmt;
    }

    // Parse write statement: write(var)
    std::shared_ptr<ASTNode> parseWriteStatement() {
        expect(TokenType::WRITE, "Expected 'write'");
        expect(TokenType::LPAREN, "Expected '(' after 'write'");

        auto writeStmt = makeNode(NodeType::EXPRESSION_STMT, "write");

        auto call = makeNode(NodeType::CALL_EXPR, "write");
        call->addChild(parseExpression());
        writeStmt->addChild(call);

        expect(TokenType::RPAREN, "Expected ')' after argument");
        match(TokenType::SEMICOLON);
        return writeStmt;
    }

    // Parse function call
    std::shared_ptr<ASTNode> parseFunctionCall() {
        std::string funcName = currentToken().value;
        advance();

        expect(TokenType::LPAREN, "Expected '(' after function name");

        auto callNode = makeNode(NodeType::CALL_EXPR, funcName);

        // Check if this function is from an imported library
        for (const auto& [libName, lib] : importedLibraries) {
            if (lib->getFunction(funcName)) {
                callNode->setAttribute("library", libName);
                callNode->setAttribute("builtin", lib->type == LibraryType::BUILTIN ? "true" : "false");
                break;
            }
        }

        // Parse arguments (if any)
        if (currentToken().type != TokenType::RPAREN) {
            while (true) {
                // named arg: IDENTIFIER '=' expr
                if (currentToken().type == TokenType::IDENTIFIER && peek().type == TokenType::ASSIGN) {
                    std::string name = currentToken().value;
                    advance();
                    expect(TokenType::ASSIGN, "Expected '=' in named argument");
                    auto named = makeNode(NodeType::NAMED_ARG, name);
                    named->addChild(parseExpression());
                    callNode->addChild(named);
                } else {
                    callNode->addChild(parseExpression());
                }

                if (!match(TokenType::COMMA)) break;
            }
        }

        expect(TokenType::RPAREN, "Expected ')' after arguments");

        return callNode;
    }

    // Parse expression (with operator precedence)
    std::shared_ptr<ASTNode> parseExpression() {
        return parseComparison();
    }

    // Comparison operators: ==, !=, <, >, <=, >=
    std::shared_ptr<ASTNode> parseComparison() {
        auto left = parseAddition();

        while (true) {
            TokenType op = currentToken().type;

            if (op == TokenType::EQUAL || op == TokenType::NOT_EQUAL ||
                op == TokenType::LESS || op == TokenType::GREATER ||
                op == TokenType::LESS_EQUAL || op == TokenType::GREATER_EQUAL) {

                std::string opStr = currentToken().value;
                advance();

                auto binOp = makeNode(NodeType::BINARY_OP, opStr);
                binOp->addChild(left);
                binOp->addChild(parseAddition());
                left = binOp;
            } else {
                break;
            }
        }

        return left;
    }

    // Addition and subtraction: +, -
    std::shared_ptr<ASTNode> parseAddition() {
        auto left = parseMultiplication();

        while (currentToken().type == TokenType::PLUS ||
               currentToken().type == TokenType::MINUS) {

            std::string op = currentToken().value;
            advance();

            auto binOp = makeNode(NodeType::BINARY_OP, op);
            binOp->addChild(left);
            binOp->addChild(parseMultiplication());
            left = binOp;
        }

        return left;
    }

    // Multiplication, division, modulo: *, /, %
    std::shared_ptr<ASTNode> parseMultiplication() {
        auto left = parseUnary();

        while (currentToken().type == TokenType::STAR ||
               currentToken().type == TokenType::SLASH ||
               currentToken().type == TokenType::PERCENT) {

            std::string op = currentToken().value;
            advance();

            auto binOp = makeNode(NodeType::BINARY_OP, op);
            binOp->addChild(left);
            binOp->addChild(parseUnary());
            left = binOp;
        }

        return left;
    }

    // Unary operators: -, +
    std::shared_ptr<ASTNode> parseUnary() {
        if (currentToken().type == TokenType::MINUS) {
            std::string op = currentToken().value;
            advance();

            auto unaryOp = makeNode(NodeType::UNARY_OP, op);
            unaryOp->addChild(parseUnary());
            return unaryOp;
        }

        if (currentToken().type == TokenType::PLUS) {
            advance(); // Just skip unary +
            return parseUnary();
        }

        return parsePrimary();
    }

    // Primary expressions: numbers, strings, identifiers, parentheses
    std::shared_ptr<ASTNode> parsePrimary() {
        // Number literal
        if (currentToken().type == TokenType::NUMBER) {
            auto literal = makeNode(NodeType::LITERAL, currentToken().value);
            if (currentToken().value.find('.') != std::string::npos) {
                literal->setAttribute("type", "float");
            } else {
                literal->setAttribute("type", "int");
            }
            advance();
            return literal;
        }

        // String literal
        if (currentToken().type == TokenType::STRING) {
            auto literal = makeNode(NodeType::LITERAL, currentToken().value);
            literal->setAttribute("type", "string");
            advance();
            return literal;
        }

        // Identifier
        if (currentToken().type == TokenType::IDENTIFIER) {
            std::string name = currentToken().value;
            advance();

            // Bool literals
            if (name == "true" || name == "false") {
                auto literal = makeNode(NodeType::LITERAL, name);
                literal->setAttribute("type", "bool");
                return literal;
            }

            // Check for function call
            if (currentToken().type == TokenType::LPAREN) {
                // Backtrack
                current--;
                return parseFunctionCall();
            }

            // Check for array access [index]
            if (currentToken().type == TokenType::LBRACKET) {
                auto identifier = makeNode(NodeType::IDENTIFIER, name);
                advance();
                auto arrayAccess = makeNode(NodeType::ARRAY_ACCESS, "[]");
                arrayAccess->addChild(identifier);
                arrayAccess->addChild(parseExpression());
                expect(TokenType::RBRACKET, "Expected ']'");
                return arrayAccess;
            }

            auto identifier = makeNode(NodeType::IDENTIFIER, name);
            return identifier;
        }

        if (match(TokenType::LBRACKET)) {
            auto arr = makeNode(NodeType::ARRAY_LITERAL, "[]");

            if (currentToken().type != TokenType::RBRACKET) {
                arr->addChild(parseExpression());
                while (match(TokenType::COMMA)) {
                    arr->addChild(parseExpression());
                }
            }

            expect(TokenType::RBRACKET, "Expected ']'");
            return arr;
        }

        // Parenthesized expression
        if (match(TokenType::LPAREN)) {
            auto expr = parseExpression();
            expect(TokenType::RPAREN, "Expected ')'");
            return expr;
        }

        throw std::runtime_error("Expected expression at line " +
                               std::to_string(currentToken().line));
    }

public:
    // Get list of user library source files that need to be compiled
    std::vector<std::string> getUserLibraryPaths() const {
        std::vector<std::string> paths;
        for (const auto& [name, lib] : importedLibraries) {
            if (lib->type == LibraryType::USER) {
                // Reconstruct the path
                std::string path = name;
                if (path.find(".cax") == std::string::npos) {
                    path += ".cax";
                }
                paths.push_back(path);
            }
        }
        return paths;
    }

    // Get all function names from imported user libraries
    std::vector<std::string> getUserLibraryFunctions() const {
        std::vector<std::string> funcNames;
        for (const auto& [libName, lib] : importedLibraries) {
            if (lib->type == LibraryType::USER) {
                for (const auto& [funcName, func] : lib->functions) {
                    funcNames.push_back(funcName);
                }
            }
        }
        return funcNames;
    }

    explicit Parser(const std::vector<Token>& toks)
        : tokens(toks), current(0) {}

    void setLibraryPath(const std::string& path) {
        libraryManager.setStdlibPath(path);
    }

    const std::map<std::string, std::shared_ptr<Library>>& getImportedLibraries() const {
        return importedLibraries;
    }

    std::shared_ptr<ASTNode> parse() {
        return parseProgram();
    }
};

} // namespace ClangAX

#endif // CLANGAX_PARSER_H