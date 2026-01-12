#ifndef CLANGAX_LEXER_H
#define CLANGAX_LEXER_H

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>

namespace ClangAX {

// Token types
enum class TokenType {
    // Keywords
    FUNC, IF, ELSE, WHILE, FOR, RETURN, PRINT, WRITE, IMPORT,

    // Literals
    NUMBER, IDENTIFIER, STRING,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQUAL, NOT_EQUAL,
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    INCREMENT, DECREMENT,  // ++ and --

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMICOLON,

    // Special
    HASH,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    std::string source;
    size_t pos;
    int line;
    int column;

    std::map<std::string, TokenType> keywords = {
        {"func", TokenType::FUNC},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"return", TokenType::RETURN},
        {"print", TokenType::PRINT},
        {"write", TokenType::WRITE},
        {"import", TokenType::IMPORT},
        {"len", TokenType::IDENTIFIER}  // len is treated as identifier (built-in function)
    };

    char current() const {
        return (pos < source.length()) ? source[pos] : '\0';
    }

    char peek(int offset = 1) const {
        size_t idx = pos + offset;
        return (idx < source.length()) ? source[idx] : '\0';
    }

    void advance() {
        if (pos < source.length()) {
            if (source[pos] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos++;
        }
    }

    void skipWhitespace() {
        while (std::isspace(current())) {
            advance();
        }
    }

    void skipComment() {
        if (current() == '/' && peek() == '/') {
            // Single-line comment
            while (current() != '\n' && current() != '\0') {
                advance();
            }
        } else if (current() == '/' && peek() == '*') {
            // Multi-line comment
            advance(); // skip '/'
            advance(); // skip '*'
            while (!(current() == '*' && peek() == '/') && current() != '\0') {
                advance();
            }
            if (current() != '\0') {
                advance(); // skip '*'
                advance(); // skip '/'
            }
        }
    }

    Token readNumber() {
        int startLine = line;
        int startColumn = column;
        std::string num;
        bool hasDecimal = false;

        while (std::isdigit(current()) || current() == '.') {
            if (current() == '.') {
                if (hasDecimal) break; // Second decimal point
                hasDecimal = true;
            }
            num += current();
            advance();
        }

        return Token(TokenType::NUMBER, num, startLine, startColumn);
    }

    Token readIdentifier() {
        int startLine = line;
        int startColumn = column;
        std::string id;

        while (std::isalnum(current()) || current() == '_') {
            id += current();
            advance();
        }

        // Check if it's a keyword
        auto it = keywords.find(id);
        TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

        return Token(type, id, startLine, startColumn);
    }

    Token readString() {
        int startLine = line;
        int startColumn = column;
        std::string str;

        advance(); // skip opening quote

        while (current() != '"' && current() != '\0') {
            if (current() == '\\' && peek() != '\0') {
                // Handle escape sequences
                advance();
                char next = current();
                switch (next) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case 'r': str += '\r'; break;
                    case '\\': str += '\\'; break;
                    case '"': str += '"'; break;
                    default: str += next; break;
                }
                advance();
            } else {
                str += current();
                advance();
            }
        }

        if (current() == '"') {
            advance(); // skip closing quote
        } else {
            std::cerr << "Unterminated string at line " << startLine << std::endl;
        }

        return Token(TokenType::STRING, str, startLine, startColumn);
    }

public:
    explicit Lexer(const std::string& src)
        : source(src), pos(0), line(1), column(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (pos < source.length()) {
            skipWhitespace();

            if (pos >= source.length()) break;

            // Skip comments
            if (current() == '/' && (peek() == '/' || peek() == '*')) {
                skipComment();
                continue;
            }

            int startLine = line;
            int startColumn = column;
            char ch = current();

            // Numbers
            if (std::isdigit(ch)) {
                tokens.push_back(readNumber());
                continue;
            }

            // Identifiers and keywords
            if (std::isalpha(ch) || ch == '_') {
                tokens.push_back(readIdentifier());
                continue;
            }

            // String literals
            if (ch == '"') {
                tokens.push_back(readString());
                continue;
            }

            // Handle # for preprocessor directives
            if (ch == '#') {
                tokens.push_back(Token(TokenType::HASH, "#", startLine, startColumn));
                advance();
                continue;
            }

            // Operators and delimiters
            switch (ch) {
                case '+':
                    if (peek() == '+') {
                        tokens.push_back(Token(TokenType::INCREMENT, "++", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::PLUS, "+", startLine, startColumn));
                        advance();
                    }
                    break;

                case '-':
                    if (peek() == '-') {
                        tokens.push_back(Token(TokenType::DECREMENT, "--", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::MINUS, "-", startLine, startColumn));
                        advance();
                    }
                    break;

                case '*':
                    tokens.push_back(Token(TokenType::STAR, "*", startLine, startColumn));
                    advance();
                    break;

                case '/':
                    tokens.push_back(Token(TokenType::SLASH, "/", startLine, startColumn));
                    advance();
                    break;

                case '%':
                    tokens.push_back(Token(TokenType::PERCENT, "%", startLine, startColumn));
                    advance();
                    break;

                case '=':
                    if (peek() == '=') {
                        tokens.push_back(Token(TokenType::EQUAL, "==", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::ASSIGN, "=", startLine, startColumn));
                        advance();
                    }
                    break;

                case '!':
                    if (peek() == '=') {
                        tokens.push_back(Token(TokenType::NOT_EQUAL, "!=", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::UNKNOWN, "!", startLine, startColumn));
                        advance();
                    }
                    break;

                case '<':
                    if (peek() == '=') {
                        tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::LESS, "<", startLine, startColumn));
                        advance();
                    }
                    break;

                case '>':
                    if (peek() == '=') {
                        tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", startLine, startColumn));
                        advance();
                        advance();
                    } else {
                        tokens.push_back(Token(TokenType::GREATER, ">", startLine, startColumn));
                        advance();
                    }
                    break;

                case '(':
                    tokens.push_back(Token(TokenType::LPAREN, "(", startLine, startColumn));
                    advance();
                    break;

                case ')':
                    tokens.push_back(Token(TokenType::RPAREN, ")", startLine, startColumn));
                    advance();
                    break;

                case '{':
                    tokens.push_back(Token(TokenType::LBRACE, "{", startLine, startColumn));
                    advance();
                    break;

                case '}':
                    tokens.push_back(Token(TokenType::RBRACE, "}", startLine, startColumn));
                    advance();
                    break;

                case '[':
                    tokens.push_back(Token(TokenType::LBRACKET, "[", startLine, startColumn));
                    advance();
                    break;

                case ']':
                    tokens.push_back(Token(TokenType::RBRACKET, "]", startLine, startColumn));
                    advance();
                    break;

                case ',':
                    tokens.push_back(Token(TokenType::COMMA, ",", startLine, startColumn));
                    advance();
                    break;

                case ';':
                    tokens.push_back(Token(TokenType::SEMICOLON, ";", startLine, startColumn));
                    advance();
                    break;

                default:
                    std::cerr << "Unknown character: '" << ch << "' at line "
                              << startLine << ", column " << startColumn << std::endl;
                    tokens.push_back(Token(TokenType::UNKNOWN, std::string(1, ch),
                                         startLine, startColumn));
                    advance();
                    break;
            }
        }

        tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
        return tokens;
    }
};

} // namespace ClangAX

#endif // CLANGAX_LEXER_H