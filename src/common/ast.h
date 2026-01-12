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

        // Types
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_BOOL,
        TYPE_ARRAY,
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