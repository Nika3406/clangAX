#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <sstream>

// Simple Symbol Table entry
struct Symbol {
    std::string name;
    std::string type;           // declared data type (may be empty if unknown)
    std::string value;          // current value as string (may be empty)
    int declared_line = -1;     // optional line number where declared (if available)
};

// Hash-based Symbol Table class
class SymbolTable {
public:
    SymbolTable() = default;

    // Insert a symbol (if not present). If present, returns false.
    bool insert(const std::string& name, const std::string& type = "", const std::string& value = "", int declared_line = -1);

    // Force insert or update (create if missing)
    void upsert(const std::string& name, const std::string& type = "", const std::string& value = "", int declared_line = -1);

    // Update only the value of an existing symbol (or create with empty type)
    void updateValue(const std::string& name, const std::string& value);

    // Update type (if you want; doesn't validate)
    void updateType(const std::string& name, const std::string& type);

    // Get a symbol (optional)
    std::optional<Symbol> get(const std::string& name) const;

    // Return a vector of all symbols (sorted by insertion order is not guaranteed)
    std::vector<Symbol> allSymbols() const;

    // Pretty print table to string
    std::string formatTable() const;

private:
    std::unordered_map<std::string, Symbol> table_;
};
#endif // SYMBOL_TABLE_H
