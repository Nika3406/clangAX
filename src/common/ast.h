#ifndef CLANGAX_AST_H
#define CLANGAX_AST_H

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace ClangAX {

    // AST Node Types
    enum class NodeType {
        // Program structure
        PROGRAM,
        FUNCTION_DECL,
        BLOCK,

        // Statements
        ASSIGNMENT,
        IF_STMT,
        WHILE_STMT,
        FOR_STMT,
        RETURN_STMT,
        PRINT_STMT,
        EXPRESSION_STMT,
        FREE_STMT,           // free(ptr) — explicit deallocation statement

        // Expressions
        BINARY_OP,
        UNARY_OP,
        POST_INCREMENT,
        LITERAL,
        IDENTIFIER,
        CALL_EXPR,
        NAMED_ARG,
        ARRAY_ACCESS,
        ARRAY_LITERAL,
        ALLOC_EXPR,          // alloc(expr) — heap-allocates a value, returns a pointer
        DEREF_EXPR,          // *ptr        — reads through a pointer
        ADDR_OF_EXPR,        // &var        — takes address of a local (returns pointer)
        DEREF_ASSIGN,        // *ptr = expr — writes a value through a pointer (statement form)

        // Types
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_BOOL,
        TYPE_ARRAY,
        TYPE_PTR,            // ptr<T>    — raw (non-owning) pointer
        TYPE_OWN,            // own<T>    — owning pointer, auto-freed at scope exit
        TYPE_BORROW,         // borrow<T> — temporary non-owning reference
    };

    // AST Node Structure
    struct ASTNode {
        NodeType type;
        std::string value;  // For identifiers, operators, literals
        std::vector<std::shared_ptr<ASTNode>> children;
        std::map<std::string, std::string> attributes;  // Extra metadata

        // Constructor
        ASTNode(NodeType t, const std::string& val = "")
            : type(t), value(val) {}

        // Helper to add child
        void addChild(std::shared_ptr<ASTNode> child) {
            children.push_back(child);
        }

        // Helper to set attribute
        void setAttribute(const std::string& key, const std::string& val) {
            attributes[key] = val;
        }

        // Helper to get attribute
        std::string getAttribute(const std::string& key) const {
            auto it = attributes.find(key);
            return (it != attributes.end()) ? it->second : "";
        }
    };

    // Helper function to create AST nodes
    inline std::shared_ptr<ASTNode> makeNode(NodeType type, const std::string& value = "") {
        return std::make_shared<ASTNode>(type, value);
    }

} // namespace ClangAX

#endif // CLANGAX_AST_H