#include "symbol_table.h"
#include <iomanip>
#include <algorithm>

bool SymbolTable::insert(const std::string& name, const std::string& type, const std::string& value, int declared_line) {
    auto it = table_.find(name);
    if (it != table_.end()) return false;
    Symbol s;
    s.name = name;
    s.type = type;
    s.value = value;
    s.declared_line = declared_line;
    table_.emplace(name, std::move(s));
    return true;
}

void SymbolTable::upsert(const std::string& name, const std::string& type, const std::string& value, int declared_line) {
    auto it = table_.find(name);
    if (it == table_.end()) {
        Symbol s;
        s.name = name;
        s.type = type;
        s.value = value;
        s.declared_line = declared_line;
        table_.emplace(name, std::move(s));
    } else {
        if (!type.empty()) it->second.type = type;
        if (!value.empty()) it->second.value = value;
        if (declared_line >= 0) it->second.declared_line = declared_line;
    }
}

void SymbolTable::updateValue(const std::string& name, const std::string& value) {
    auto it = table_.find(name);
    if (it == table_.end()) {
        Symbol s;
        s.name = name;
        s.type = "";
        s.value = value;
        table_.emplace(name, std::move(s));
    } else {
        it->second.value = value;
    }
}

void SymbolTable::updateType(const std::string& name, const std::string& type) {
    auto it = table_.find(name);
    if (it == table_.end()) {
        Symbol s;
        s.name = name;
        s.type = type;
        table_.emplace(name, std::move(s));
    } else {
        it->second.type = type;
    }
}

std::optional<Symbol> SymbolTable::get(const std::string& name) const {
    auto it = table_.find(name);
    if (it == table_.end()) return std::nullopt;
    return it->second;
}

std::vector<Symbol> SymbolTable::allSymbols() const {
    std::vector<Symbol> out;
    out.reserve(table_.size());
    for (const auto& [k, v] : table_) out.push_back(v);
    // Optional: sort alphabetically for deterministic output
    std::sort(out.begin(), out.end(), [](const Symbol& a, const Symbol& b) {
        return a.name < b.name;
    });
    return out;
}

std::string SymbolTable::formatTable() const {
    auto rows = allSymbols();
    std::stringstream ss;
    ss << "Symbol Table (" << rows.size() << " entries)\n";
    ss << "-------------------------------------------\n";
    ss << std::left << std::setw(20) << "Name" << std::setw(12) << "Type" << std::setw(16) << "Value" << "Line\n";
    ss << "-------------------------------------------\n";
    for (const auto& s : rows) {
        ss << std::left << std::setw(20) << s.name << std::setw(12) << (s.type.empty() ? "(none)" : s.type)
           << std::setw(16) << (s.value.empty() ? "(unset)" : s.value)
           << (s.declared_line >= 0 ? std::to_string(s.declared_line) : "-") << "\n";
    }
    if (rows.empty()) ss << "(empty)\n";
    return ss.str();
}
