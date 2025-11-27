#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <functional>

using namespace std;


// ============================================
// TOKEN DEFINITIONS
// ============================================

enum class TokenType {
    // Literals
    INTEGER, FLOAT, STRING, CHAR, BOOLEAN, NUL,

    // Identifiers and Keywords
    IDENTIFIER, IMPORT, EXEC, FUNC, CLASS, OBJECT, MEMBER,
    FOR, WHILE, IF, ELSE, IN, RANGE, RETURN,
    PRINT, VECTOR, PUSH, POP, SIZE, LEN,
    TRUE, FALSE, NULL_KW,

    // Operators
    PLUS, MINUS, MULT, DIV, MOD, DOT,
    ASSIGN, EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,
    INC, DEC, PLUS_EQ, MINUS_EQ, MULT_EQ, DIV_EQ,

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, HASH,

    // Special
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

// ============================================
// AST NODE DEFINITIONS
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
// LEXER
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
// PARSER
// ============================================

class Parser {
private:
    vector<Token> tokens;
    size_t current;
    vector<string> errors;

    void debugToken(const string& where) {
        const Token& t = peek();
        cerr << "[DEBUG] " << where << " → Token("
             << (int)t.type << ", '" << t.value << "', line " << t.line << ")\n";
    }

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

    shared_ptr<ASTNode> parseProgram() {
        auto program = make_shared<ASTNode>(NodeType::PROGRAM, "program");

        while (peek().type != TokenType::END_OF_FILE) {
            debugToken("parseProgram loop");
            if (peek().type == TokenType::HASH) {
                advance();
                if (peek().type == TokenType::IMPORT) {
                    program->addChild(parseImport());
                }
            } else if (match(TokenType::EXEC)){
                program->addChild(parseExec());
            } else if (peek().type == TokenType::FUNC) {
                program->addChild(parseFunction());
            } else if (peek().type == TokenType::CLASS) {
                program->addChild(parseClass());
            } else {
                advance();
            }
        }

        return program;
    }

    shared_ptr<ASTNode> parseImport() {
        expect(TokenType::IMPORT, "Expected 'import'");
        Token module = expect(TokenType::STRING, "Expected module name");

        auto node = make_shared<ASTNode>(NodeType::IMPORT_STMT, module.value, module.line);
        return node;
    }

    shared_ptr<ASTNode> parseExec() {
        // EXEC has already been consumed by match() in parseProgram()
        Token execToken = tokens[current - 1];
        expect(TokenType::LPAREN, "Expected '(' after exec");

        auto node = make_shared<ASTNode>(NodeType::EXEC_STMT, "exec", execToken.line);

        int safety_counter = 0;
        const int MAX_ITER = 10000;

        while (peek().type != TokenType::RPAREN &&
               peek().type != TokenType::END_OF_FILE) {

            if (++safety_counter > MAX_ITER) {
                errors.push_back("Line " + to_string(peek().line) + ": Too many iterations parsing exec (possible infinite loop)");
                break;
            }

            // Handle named parameter: IDENTIFIER '=' expression
            if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::ASSIGN) {
                Token param = advance();              // identifier
                expect(TokenType::ASSIGN, "Expected '=' in exec");
                auto valueNode = parseExpression();   // parse the value as a full expression

                auto paramNode = make_shared<ASTNode>(NodeType::ASSIGNMENT, param.value);
                if (valueNode) paramNode->addChild(valueNode);
                node->addChild(paramNode);
            } else {
                // Positional value (could be literal, identifier, function call, etc.)
                // Use parseExpression to consume a valid expression/value.
                auto valueNode = parseExpression();
                if (valueNode) {
                    node->addChild(valueNode);
                } else {
                    // Fallback: if parseExpression didn't consume anything, advance to avoid hang
                    if (peek().type == TokenType::COMMA) {
                        // nothing to add, will skip comma below
                    } else if (peek().type == TokenType::END_OF_FILE || peek().type == TokenType::RPAREN) {
                        // nothing to do
                    } else {
                        // Skip unexpected token to prevent infinite loop
                        advance();
                    }
                }
            }

            // Skip optional comma separator between parameters
            if (peek().type == TokenType::COMMA) advance();
        }

        expect(TokenType::RPAREN, "Expected ')' after exec");

        return node;
    }


    shared_ptr<ASTNode> parseFunction() {
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

        auto node = make_shared<ASTNode>(NodeType::FUNCTION_DECL,
                                        funcName.empty() ? funcType : funcName,
                                        funcToken.line);
        if (!funcType.empty()) {
            node->setAttribute("type", funcType);
        }

        auto body = parseBlock();
        node->addChild(body);

        return node;
    }

    shared_ptr<ASTNode> parseClass() {
        Token classToken = expect(TokenType::CLASS, "Expected 'class'");
        expect(TokenType::LPAREN, "Expected '(' after class");

        string classType = "";
        if (peek().type == TokenType::IDENTIFIER) {
            classType = advance().value;
        }

        expect(TokenType::RPAREN, "Expected ')' after class type");
        expect(TokenType::ASSIGN, "Expected '=' after class()");

        Token nameToken = expect(TokenType::STRING, "Expected class name");
        string className = nameToken.value;

        auto node = make_shared<ASTNode>(NodeType::CLASS_DECL, className, classToken.line);
        if (!classType.empty()) {
            node->setAttribute("type", classType);
        }

        expect(TokenType::LBRACE, "Expected '{' after class declaration");

        if (match(TokenType::OBJECT)) {
            expect(TokenType::COLON, "Expected ':' after object");
            auto objSection = make_shared<ASTNode>(NodeType::OBJECT_SECTION, "object");

            while (peek().type != TokenType::MEMBER &&
                   peek().type != TokenType::RBRACE &&
                   peek().type != TokenType::END_OF_FILE) {
                if (peek().type == TokenType::IDENTIFIER) {
                    Token var = advance();
                    auto varNode = make_shared<ASTNode>(NodeType::IDENTIFIER, var.value);
                    objSection->addChild(varNode);
                }
            }
            node->addChild(objSection);
        }

        if (match(TokenType::MEMBER)) {
            expect(TokenType::COLON, "Expected ':' after member");
            auto memSection = make_shared<ASTNode>(NodeType::MEMBER_SECTION, "member");

            while (peek().type != TokenType::RBRACE && peek().type != TokenType::END_OF_FILE) {
                if (peek().type == TokenType::FUNC) {
                    memSection->addChild(parseFunction());
                } else {
                    advance();
                }
            }
            node->addChild(memSection);
        }

        expect(TokenType::RBRACE, "Expected '}' after class body");
        return node;
    }

    shared_ptr<ASTNode> parseBlock() {
        expect(TokenType::LBRACE, "Expected '{'");
        auto block = make_shared<ASTNode>(NodeType::BLOCK, "block");

        int safety_counter = 0;
        const int MAX_STATEMENTS = 10000;

        while (peek().type != TokenType::RBRACE && peek().type != TokenType::END_OF_FILE) {
            debugToken("parseBlock loop");
            if (++safety_counter > MAX_STATEMENTS) {
                errors.push_back("Line " + to_string(peek().line) + ": Too many statements in block (possible infinite loop)");
                break;
            }

            auto stmt = parseStatement();
            if (stmt) {
                block->addChild(stmt);
            }
        }

        expect(TokenType::RBRACE, "Expected '}'");
        return block;
    }

    shared_ptr<ASTNode> parseStatement() {
        debugToken("parseStatement enter");
        // Skip any unexpected tokens at statement level
        if (peek().type == TokenType::RBRACE || peek().type == TokenType::END_OF_FILE) {
            return nullptr;  // Changed from creating empty node to returning nullptr
        }

        Token currentToken = peek();

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
            // Check if it's an assignment or just an expression
            TokenType nextType = peek(1).type;
            if (nextType == TokenType::ASSIGN ||
                nextType == TokenType::PLUS_EQ ||
                nextType == TokenType::MINUS_EQ ||
                nextType == TokenType::MULT_EQ ||
                nextType == TokenType::DIV_EQ) {
                return parseAssignment();
            } else if (nextType == TokenType::LBRACKET) {
                // Could be array access assignment like arr[0] = 5
                size_t saved = current;
                advance(); // skip identifier
                advance(); // skip [

                // Skip to find ]
                int bracketDepth = 1;
                while (bracketDepth > 0 && peek().type != TokenType::END_OF_FILE) {
                    if (peek().type == TokenType::LBRACKET) bracketDepth++;
                    else if (peek().type == TokenType::RBRACKET) bracketDepth--;
                    advance();
                }

                // Check if followed by assignment
                TokenType afterBracket = peek().type;
                current = saved; // restore position

                if (afterBracket == TokenType::ASSIGN || afterBracket == TokenType::PLUS_EQ ||
                    afterBracket == TokenType::MINUS_EQ || afterBracket == TokenType::MULT_EQ ||
                    afterBracket == TokenType::DIV_EQ) {
                    return parseAssignment();
                }
            }
            // If it's just an identifier (like a function call), parse as expression
            return parseExpression();
        }

        // Try to parse as expression (handles function calls, operators, etc.)
        return parseExpression();
    }

    shared_ptr<ASTNode> parseFor() {
        Token forToken = expect(TokenType::FOR, "Expected 'for'");
        expect(TokenType::LPAREN, "Expected '(' after for");

        auto node = make_shared<ASTNode>(NodeType::FOR_STMT, "for", forToken.line);

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

        node->addChild(parseExpression());
        expect(TokenType::COMMA, "Expected ',' in for");
        node->addChild(parseExpression());
        expect(TokenType::COMMA, "Expected ',' in for");
        node->addChild(parseExpression());
        expect(TokenType::RPAREN, "Expected ')' after for");

        node->addChild(parseBlock());
        return node;
    }

    shared_ptr<ASTNode> parseWhile() {
        Token whileToken = expect(TokenType::WHILE, "Expected 'while'");
        expect(TokenType::LPAREN, "Expected '(' after while");

        auto node = make_shared<ASTNode>(NodeType::WHILE_STMT, "while", whileToken.line);
        node->addChild(parseExpression());

        expect(TokenType::RPAREN, "Expected ')' after while condition");
        node->addChild(parseBlock());

        return node;
    }

    shared_ptr<ASTNode> parseIf() {
        Token ifToken = expect(TokenType::IF, "Expected 'if'");
        expect(TokenType::LPAREN, "Expected '(' after if");

        auto node = make_shared<ASTNode>(NodeType::IF_STMT, "if", ifToken.line);
        node->addChild(parseExpression());

        expect(TokenType::RPAREN, "Expected ')' after if condition");
        node->addChild(parseBlock());

        if (match(TokenType::ELSE)) {
            node->addChild(parseBlock());
        }

        return node;
    }

    shared_ptr<ASTNode> parseReturn() {
        Token retToken = expect(TokenType::RETURN, "Expected 'return'");
        auto node = make_shared<ASTNode>(NodeType::RETURN_STMT, "return", retToken.line);
        node->addChild(parseExpression());
        return node;
    }

    shared_ptr<ASTNode> parsePrint() {
        Token printToken = expect(TokenType::PRINT, "Expected 'print'");
        expect(TokenType::LPAREN, "Expected '(' after print");

        auto node = make_shared<ASTNode>(NodeType::PRINT_STMT, "print", printToken.line);

        if (peek().type != TokenType::RPAREN) {
            node->addChild(parseExpression());

            while (match(TokenType::COMMA)) {
                node->addChild(parseExpression());
            }
        }

        expect(TokenType::RPAREN, "Expected ')' after print");
        return node;
    }

    shared_ptr<ASTNode> parseVectorDecl() {
        expect(TokenType::VECTOR, "Expected 'vector'");
        expect(TokenType::LT, "Expected '<' after vector");
        Token type = expect(TokenType::IDENTIFIER, "Expected type");
        expect(TokenType::GT, "Expected '>' after type");
        Token name = expect(TokenType::IDENTIFIER, "Expected identifier");

        auto node = make_shared<ASTNode>(NodeType::VECTOR_DECL, name.value);
        node->setAttribute("elementType", type.value);

        return node;
    }

    shared_ptr<ASTNode> parseAssignment() {
        Token var = expect(TokenType::IDENTIFIER, "Expected identifier");

        if (peek().type == TokenType::LBRACKET) {
            advance();
            auto indexNode = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']'");

            TokenType assignType = peek().type;
            if (assignType == TokenType::ASSIGN || assignType == TokenType::PLUS_EQ ||
                assignType == TokenType::MINUS_EQ || assignType == TokenType::MULT_EQ ||
                assignType == TokenType::DIV_EQ) {
                Token op = advance();
                auto node = make_shared<ASTNode>(NodeType::ASSIGNMENT, var.value);
                node->setAttribute("operator", op.value);
                node->addChild(indexNode);
                node->addChild(parseExpression());
                return node;
            }
        }

        TokenType assignType = peek().type;
        if (assignType == TokenType::PLUS_EQ || assignType == TokenType::MINUS_EQ ||
            assignType == TokenType::MULT_EQ || assignType == TokenType::DIV_EQ) {
            Token op = advance();
            auto node = make_shared<ASTNode>(NodeType::ASSIGNMENT, var.value);
            node->setAttribute("operator", op.value);
            node->addChild(parseExpression());
            return node;
        }

        expect(TokenType::ASSIGN, "Expected '='");

        auto node = make_shared<ASTNode>(NodeType::ASSIGNMENT, var.value);
        node->addChild(parseExpression());

        return node;
    }

    shared_ptr<ASTNode> parseExpression() {
        return parseLogicalOr();
    }

    shared_ptr<ASTNode> parseLogicalOr() {
        auto left = parseLogicalAnd();

        while (match(TokenType::OR)) {
            auto node = make_shared<ASTNode>(NodeType::BINARY_OP, "||");
            node->addChild(left);
            node->addChild(parseLogicalAnd());
            left = node;
        }

        return left;
    }

    shared_ptr<ASTNode> parseLogicalAnd() {
        auto left = parseEquality();

        while (match(TokenType::AND)) {
            auto node = make_shared<ASTNode>(NodeType::BINARY_OP, "&&");
            node->addChild(left);
            node->addChild(parseEquality());
            left = node;
        }

        return left;
    }

    shared_ptr<ASTNode> parseEquality() {
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

    shared_ptr<ASTNode> parseComparison() {
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

    shared_ptr<ASTNode> parseTerm() {
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

    shared_ptr<ASTNode> parseFactor() {
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

    shared_ptr<ASTNode> parseUnary() {
        if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS ||
            peek().type == TokenType::INC || peek().type == TokenType::DEC) {
            Token op = advance();
            auto node = make_shared<ASTNode>(NodeType::UNARY_OP, op.value);
            node->addChild(parseUnary());
            return node;
        }

        return parsePostfix();
    }

    shared_ptr<ASTNode> parsePostfix() {
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
                Token member = expect(TokenType::IDENTIFIER, "Expected member name");
                auto node = make_shared<ASTNode>(NodeType::MEMBER_ACCESS, member.value);
                node->addChild(expr);

                if (peek().type == TokenType::LPAREN) {
                    advance();
                    auto callNode = make_shared<ASTNode>(NodeType::FUNCTION_CALL, member.value);
                    callNode->addChild(expr);

                    while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
                        callNode->addChild(parseExpression());
                        if (peek().type == TokenType::COMMA) advance();
                    }

                    expect(TokenType::RPAREN, "Expected ')' after function call");
                    expr = callNode;
                } else {
                    expr = node;
                }
            } else if (match(TokenType::LBRACKET)) {
                auto node = make_shared<ASTNode>(NodeType::ARRAY_ACCESS, "[]");
                node->addChild(expr);
                node->addChild(parseExpression());
                expect(TokenType::RBRACKET, "Expected ']'");
                expr = node;
            } else if (peek().type == TokenType::LPAREN && expr->type == NodeType::IDENTIFIER) {
                advance();
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

    shared_ptr<ASTNode> parsePrimary() {
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

        // CRITICAL: Must advance to prevent infinite loop
        Token badToken = peek();
        errors.push_back("Line " + to_string(badToken.line) + ": Unexpected token: " + badToken.value);
        advance();  // MUST advance here to prevent infinite loop
        return make_shared<ASTNode>(NodeType::LITERAL, "error");
        debugToken("parsePrimary BAD");
    }

public:
    Parser(const vector<Token>& toks) : tokens(toks), current(0) {}

    shared_ptr<ASTNode> parse() {
        return parseProgram();
    }

    vector<string> getErrors() const {
        return errors;
    }
};

// ============================================
// AST PRINTER
// ============================================

class ASTPrinter {
private:
    int indentLevel = 0;

    string getIndent() {
        return string(indentLevel * 2, ' ');
    }

    string nodeTypeToString(NodeType type) {
        switch (type) {
            case NodeType::PROGRAM: return "PROGRAM";
            case NodeType::IMPORT_STMT: return "IMPORT";
            case NodeType::EXEC_STMT: return "EXEC";
            case NodeType::FUNCTION_DECL: return "FUNCTION";
            case NodeType::CLASS_DECL: return "CLASS";
            case NodeType::OBJECT_SECTION: return "OBJECT_SECTION";
            case NodeType::MEMBER_SECTION: return "MEMBER_SECTION";
            case NodeType::BLOCK: return "BLOCK";
            case NodeType::ASSIGNMENT: return "ASSIGNMENT";
            case NodeType::VAR_DECL: return "VAR_DECL";
            case NodeType::VECTOR_DECL: return "VECTOR_DECL";
            case NodeType::IF_STMT: return "IF";
            case NodeType::WHILE_STMT: return "WHILE";
            case NodeType::FOR_STMT: return "FOR";
            case NodeType::RANGE_FOR: return "RANGE_FOR";
            case NodeType::RETURN_STMT: return "RETURN";
            case NodeType::PRINT_STMT: return "PRINT";
            case NodeType::BINARY_OP: return "BINARY_OP";
            case NodeType::UNARY_OP: return "UNARY_OP";
            case NodeType::FUNCTION_CALL: return "FUNCTION_CALL";
            case NodeType::MEMBER_ACCESS: return "MEMBER_ACCESS";
            case NodeType::ARRAY_ACCESS: return "ARRAY_ACCESS";
            case NodeType::ARRAY_LITERAL: return "ARRAY_LITERAL";
            case NodeType::LITERAL: return "LITERAL";
            case NodeType::IDENTIFIER: return "IDENTIFIER";
            default: return "UNKNOWN";
        }
    }

public:
    void print(shared_ptr<ASTNode> node, ostream& out) {
        if (!node) return;

        out << getIndent() << nodeTypeToString(node->type);

        if (!node->value.empty()) {
            out << ": " << node->value;
        }

        if (!node->attributes.empty()) {
            out << " [";
            bool first = true;
            for (const auto& attr : node->attributes) {
                if (!first) out << ", ";
                out << attr.first << "=" << attr.second;
                first = false;
            }
            out << "]";
        }

        if (node->line > 0) {
            out << " (line " << node->line << ")";
        }

        out << "\n";

        indentLevel++;
        for (auto& child : node->children) {
            print(child, out);
        }
        indentLevel--;
    }
};

// ============================================
// PARSE STATISTICS
// ============================================

struct ParseStatistics {
    int totalNodes = 0;
    int functions = 0;
    int classes = 0;
    int imports = 0;
    int execs = 0;
    int assignments = 0;
    int forLoops = 0;
    int whileLoops = 0;
    int ifStatements = 0;
    int functionCalls = 0;
    int binaryOps = 0;
    int unaryOps = 0;

    void collect(shared_ptr<ASTNode> node) {
        if (!node) return;

        totalNodes++;

        switch (node->type) {
            case NodeType::FUNCTION_DECL: functions++; break;
            case NodeType::CLASS_DECL: classes++; break;
            case NodeType::IMPORT_STMT: imports++; break;
            case NodeType::EXEC_STMT: execs++; break;
            case NodeType::ASSIGNMENT: assignments++; break;
            case NodeType::FOR_STMT: forLoops++; break;
            case NodeType::WHILE_STMT: whileLoops++; break;
            case NodeType::IF_STMT: ifStatements++; break;
            case NodeType::FUNCTION_CALL: functionCalls++; break;
            case NodeType::BINARY_OP: binaryOps++; break;
            case NodeType::UNARY_OP: unaryOps++; break;
            default: break;
        }

        for (auto& child : node->children) {
            collect(child);
        }
    }

    void print(ostream& out) {
        out << "\n====================================\n";
        out << "PARSE TREE STATISTICS\n";
        out << "====================================\n";
        out << "Total AST Nodes: " << totalNodes << "\n";
        out << "Imports: " << imports << "\n";
        out << "Exec Directives: " << execs << "\n";
        out << "Functions: " << functions << "\n";
        out << "Classes: " << classes << "\n";
        out << "Assignments: " << assignments << "\n";
        out << "For Loops: " << forLoops << "\n";
        out << "While Loops: " << whileLoops << "\n";
        out << "If Statements: " << ifStatements << "\n";
        out << "Function Calls: " << functionCalls << "\n";
        out << "Binary Operations: " << binaryOps << "\n";
        out << "Unary Operations: " << unaryOps << "\n";
        out << "====================================\n\n";
    }
};

// ============================================
// MAIN
// ============================================

int main(int argc, char* argv[]) {
    string filename = (argc < 2) ? "../SampleCode.txt" : argv[1];

    cout << "C-Accel Syntax Parser\n";
    cout << "=====================\n";
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
        cout << "======================\n";
        for (const auto& error : errors) {
            cout << error << "\n";
        }
        cout << "\n";
    } else {
        cout << "Parsing completed successfully with no errors!\n\n";
    }

    ParseStatistics stats;
    stats.collect(ast);
    stats.print(cout);

    cout << "ABSTRACT SYNTAX TREE:\n";
    cout << "=====================\n";
    ASTPrinter printer;
    printer.print(ast, cout);

    ofstream astFile("../parser/parse_tree.txt");
    if (astFile.is_open()) {
        astFile << "C-ACCEL PARSE TREE\n";
        astFile << "==================\n\n";
        printer.print(ast, astFile);
        astFile << "\n\n";
        stats.print(astFile);
        astFile.close();
        cout << "\nParse tree saved to: ../parser/parse_tree.txt\n";
    }

    ofstream reportFile("../parser/parse_report.txt");
    if (reportFile.is_open()) {
        reportFile << "C-ACCEL SYNTAX PARSE REPORT\n";
        reportFile << "===========================\n";
        reportFile << "Source File: " << filename << "\n";
        reportFile << "Total Tokens: " << tokens.size() << "\n\n";

        if (!errors.empty()) {
            reportFile << "ERRORS:\n";
            for (const auto& error : errors) {
                reportFile << "  " << error << "\n";
            }
            reportFile << "\n";
        } else {
            reportFile << "Status: Parsing completed successfully!\n\n";
        }

        stats.print(reportFile);

        reportFile << "\nKEY CONSTRUCTS FOUND:\n";
        reportFile << "====================\n";

        function<void(shared_ptr<ASTNode>, int)> listFunctions;
        listFunctions = [&](shared_ptr<ASTNode> node, int depth) {
            if (!node) return;

            if (node->type == NodeType::FUNCTION_DECL) {
                string type = node->attributes.count("type") ?
                             " [" + node->attributes["type"] + "]" : "";
                reportFile << "  Function: " << node->value << type
                          << " (line " << node->line << ")\n";
            }

            if (node->type == NodeType::CLASS_DECL) {
                string type = node->attributes.count("type") ?
                             " [" + node->attributes["type"] + "]" : "";
                reportFile << "  Class: " << node->value << type
                          << " (line " << node->line << ")\n";
            }

            for (auto& child : node->children) {
                listFunctions(child, depth + 1);
            }
        };

        listFunctions(ast, 0);

        reportFile.close();
        cout << "Parse report saved to: ../parser/parse_report.txt\n";
    }

    cout << "\nParser execution completed.\n";
    return 0;
}