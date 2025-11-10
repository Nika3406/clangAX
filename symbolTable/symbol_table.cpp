#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <variant>

using namespace std;
namespace fs = std::filesystem;

// Variant type to hold different value types
using VarValue = variant<int, double, string, char, monostate>;

// Symbol Table Entry
struct SymbolEntry {
    string name;
    string dataType;
    VarValue value;
    int line_declared;
    bool initialized;

    SymbolEntry() : name(""), dataType(""), value(monostate{}), line_declared(0), initialized(false) {}

    SymbolEntry(string n, string t, int line)
        : name(n), dataType(t), value(monostate{}), line_declared(line), initialized(false) {}
};

// Hash Table based Symbol Table
class SymbolTable {
private:
    static const int TABLE_SIZE = 101; // Prime number for better distribution
    vector<vector<SymbolEntry>> table;

    // Simple hash function
    int hashFunction(const string& key) {
        int hash = 0;
        for (char c : key) {
            hash = (hash * 31 + c) % TABLE_SIZE;
        }
        return hash;
    }

public:
    SymbolTable() : table(TABLE_SIZE) {}

    // Insert or update symbol
    bool insert(const string& name, const string& dataType, int line) {
        int index = hashFunction(name);

        // Check if symbol already exists
        for (auto& entry : table[index]) {
            if (entry.name == name) {
                return false; // Duplicate declaration
            }
        }

        // Create new entry
        SymbolEntry entry(name, dataType, line);
        table[index].push_back(entry);
        return true;
    }

    // Update value of existing symbol
    bool updateValue(const string& name, const VarValue& val) {
        int index = hashFunction(name);

        for (auto& entry : table[index]) {
            if (entry.name == name) {
                entry.value = val;
                entry.initialized = true;
                return true;
            }
        }
        return false; // Symbol not found
    }

    // Lookup symbol
    SymbolEntry* lookup(const string& name) {
        int index = hashFunction(name);

        for (auto& entry : table[index]) {
            if (entry.name == name) {
                return &entry;
            }
        }
        return nullptr;
    }

    // Get all symbols sorted by name
    vector<SymbolEntry*> getAllSymbols() {
        vector<SymbolEntry*> symbols;
        for (auto& bucket : table) {
            for (auto& entry : bucket) {
                symbols.push_back(&entry);
            }
        }

        // Sort by name
        sort(symbols.begin(), symbols.end(),
             [](SymbolEntry* a, SymbolEntry* b) { return a->name < b->name; });

        return symbols;
    }

    // Print symbol table
    void print(const string& title) {
        cout << "\n" << string(80, '=') << "\n";
        cout << title << "\n";
        cout << string(80, '=') << "\n";

        auto symbols = getAllSymbols();

        if (symbols.empty()) {
            cout << "Symbol table is empty.\n";
            return;
        }

        cout << left << setw(20) << "Variable Name"
             << setw(15) << "Data Type"
             << setw(20) << "Current Value"
             << setw(10) << "Line"
             << setw(15) << "Initialized" << "\n";
        cout << string(80, '-') << "\n";

        for (auto* entry : symbols) {
            cout << left << setw(20) << entry->name
                 << setw(15) << entry->dataType;

            // Print value based on type
            string valueStr;
            if (entry->initialized) {
                if (holds_alternative<int>(entry->value)) {
                    valueStr = to_string(get<int>(entry->value));
                } else if (holds_alternative<double>(entry->value)) {
                    valueStr = to_string(get<double>(entry->value));
                } else if (holds_alternative<string>(entry->value)) {
                    valueStr = get<string>(entry->value);
                } else if (holds_alternative<char>(entry->value)) {
                    valueStr = string(1, get<char>(entry->value));
                } else {
                    valueStr = "(uninitialized)";
                }
            } else {
                valueStr = "(uninitialized)";
            }

            cout << setw(20) << valueStr
                 << setw(10) << entry->line_declared
                 << setw(15) << (entry->initialized ? "Yes" : "No") << "\n";
        }
        cout << "\nTotal symbols: " << symbols.size() << "\n";
    }
};

// Language specifications
const set<string> SPEC_DATATYPES = {
    "int", "float", "char", "string", "array", "matrix", "object", "dataset", "neuro"
};

const set<string> SPEC_RESERVED = {
    "func", "class", "object", "member", "import", "exec",
    "for", "while", "if", "else", "in", "range", "return", "print"
};

// Strip comments
string stripComment(const string& line) {
    size_t pos = line.find("//");
    if (pos == string::npos) return line;
    return line.substr(0, pos);
}

// Parse value from assignment
VarValue parseValue(const string& valueStr) {
    string trimmed = valueStr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

    // String literal
    if (trimmed.front() == '"' && trimmed.back() == '"') {
        return trimmed;
    }

    // Char literal
    if (trimmed.front() == '\'' && trimmed.back() == '\'') {
        if (trimmed.length() >= 3) {
            return trimmed[1];
        }
    }

    // Number
    if (trimmed.find('.') != string::npos) {
        try {
            return stod(trimmed);
        } catch (...) {}
    } else {
        try {
            return stoi(trimmed);
        } catch (...) {}
    }

    return monostate{}; // Unable to parse
}

// Process source code and build symbol table
void processSourceCode(const string& src, SymbolTable& symTable) {
    regex RE_TYPED_DECL(R"(\b(int|float|char|string|array|matrix|object|dataset|neuro)\s+([A-Za-z_]\w*)\s*=\s*(.+))");
    regex RE_ASSIGNMENT(R"(([A-Za-z_]\w*)\s*=\s*(.+))");

    stringstream ss(src);
    string line;
    int lineNum = 0;

    while (getline(ss, line)) {
        lineNum++;
        string cleaned = stripComment(line);

        if (cleaned.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // Check for typed declaration with assignment
        smatch match;
        if (regex_search(cleaned, match, RE_TYPED_DECL)) {
            string dataType = match[1];
            string varName = match[2];
            string valueStr = match[3];

            // Remove trailing characters
            size_t semicolon = valueStr.find(';');
            if (semicolon != string::npos) {
                valueStr = valueStr.substr(0, semicolon);
            }

            symTable.insert(varName, dataType, lineNum);
            VarValue val = parseValue(valueStr);
            symTable.updateValue(varName, val);
        }
        // Check for simple assignment (inferred type)
        else if (regex_search(cleaned, match, RE_ASSIGNMENT)) {
            string varName = match[1];
            string valueStr = match[2];

            // Skip if it's a reserved word
            if (SPEC_RESERVED.count(varName)) continue;

            // Remove trailing characters
            size_t semicolon = valueStr.find(';');
            if (semicolon != string::npos) {
                valueStr = valueStr.substr(0, semicolon);
            }

            // Check if variable exists, otherwise infer type
            SymbolEntry* existing = symTable.lookup(varName);
            if (!existing) {
                VarValue val = parseValue(valueStr);
                string inferredType = "unknown";

                if (holds_alternative<int>(val)) inferredType = "int";
                else if (holds_alternative<double>(val)) inferredType = "float";
                else if (holds_alternative<string>(val)) inferredType = "string";
                else if (holds_alternative<char>(val)) inferredType = "char";

                symTable.insert(varName, inferredType, lineNum);
                symTable.updateValue(varName, val);
            } else {
                // Update existing variable
                VarValue val = parseValue(valueStr);
                symTable.updateValue(varName, val);
            }
        }
    }
}

// Simulate some updates to demonstrate table changes
void simulateUpdates(SymbolTable& symTable) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "SIMULATING VARIABLE UPDATES\n";
    cout << string(80, '=') << "\n";

    // Update some variables with new values
    struct Update {
        string name;
        VarValue newValue;
    };

    vector<Update> updates = {
        {"w", 100},
        {"s", 9.99},
        {"pingas", 0},
        {"y", string("\"Updated String\"")},
        {"z", 'X'}
    };

    for (const auto& upd : updates) {
        if (symTable.updateValue(upd.name, upd.newValue)) {
            cout << "Updated '" << upd.name << "' with new value\n";
        }
    }
}

int main(int argc, char* argv[]) {
    string filename;

    if (argc < 2) {
        filename = "../SampleCode.txt";
        cout << "No source file provided. Using default: " << filename << endl;
    } else {
        filename = argv[1];
    }

    ifstream file(filename);

    if (!file.is_open()) {
        fs::path alt = fs::path("..") / filename;
        if (fs::exists(alt)) {
            cout << "File not found in current directory. Using parent: " << alt << endl;
            file.open(alt);
        }
    }

    if (!file.is_open()) {
        cerr << "Error: could not open source file (" << filename << ")" << endl;
        return 2;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    string src = buffer.str();

    // Create symbol table
    SymbolTable symTable;

    // Process source code
    cout << "\nProcessing source code: " << filename << "\n";
    processSourceCode(src, symTable);

    // Print initial state
    symTable.print("INITIAL SYMBOL TABLE STATE");

    // Simulate some updates
    simulateUpdates(symTable);

    // Print updated state
    symTable.print("UPDATED SYMBOL TABLE STATE");

    // Save report
    fs::path current = fs::current_path();
    fs::path project_root = current.parent_path();
    fs::path output_path = project_root / "symbolTable" / "symbol_table_report.txt";

    // Redirect cout to file
    streambuf* coutbuf = cout.rdbuf();
    ofstream output_file(output_path);

    if (output_file.is_open()) {
        cout.rdbuf(output_file.rdbuf());

        symTable.print("INITIAL SYMBOL TABLE STATE");
        cout << "\n\n";
        symTable.print("UPDATED SYMBOL TABLE STATE");

        cout.rdbuf(coutbuf);
        output_file.close();

        cout << "\n\nSymbol table report saved to: " << output_path << endl;
    }
    
    return 0;
}