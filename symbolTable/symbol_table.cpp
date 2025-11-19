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
    string scope;  // global, function_name, class_name

    SymbolEntry() : name(""), dataType(""), value(monostate{}), line_declared(0), initialized(false), scope("global") {}

    SymbolEntry(string n, string t, int line, string sc = "global")
        : name(n), dataType(t), value(monostate{}), line_declared(line), initialized(false), scope(sc) {}
};

// Hash Table based Symbol Table
class SymbolTable {
private:
    static const int TABLE_SIZE = 101;
    vector<vector<SymbolEntry>> table;

    int hashFunction(const string& key) {
        int hash = 0;
        for (char c : key) {
            hash = (hash * 31 + c) % TABLE_SIZE;
        }
        return hash;
    }

public:
    SymbolTable() : table(TABLE_SIZE) {}

    bool insert(const string& name, const string& dataType, int line, const string& scope = "global") {
        int index = hashFunction(name);

        // Check if symbol already exists in the same scope
        for (auto& entry : table[index]) {
            if (entry.name == name && entry.scope == scope) {
                return false; // Duplicate in same scope
            }
        }

        SymbolEntry entry(name, dataType, line, scope);
        table[index].push_back(entry);
        return true;
    }

    bool updateValue(const string& name, const VarValue& val, const string& scope = "global") {
        int index = hashFunction(name);

        for (auto& entry : table[index]) {
            if (entry.name == name && entry.scope == scope) {
                entry.value = val;
                entry.initialized = true;
                return true;
            }
        }
        return false;
    }

    SymbolEntry* lookup(const string& name, const string& scope = "global") {
        int index = hashFunction(name);

        for (auto& entry : table[index]) {
            if (entry.name == name && entry.scope == scope) {
                return &entry;
            }
        }
        return nullptr;
    }

    vector<SymbolEntry*> getAllSymbols() {
        vector<SymbolEntry*> symbols;
        for (auto& bucket : table) {
            for (auto& entry : bucket) {
                symbols.push_back(&entry);
            }
        }

        sort(symbols.begin(), symbols.end(),
             [](SymbolEntry* a, SymbolEntry* b) {
                 if (a->scope != b->scope) return a->scope < b->scope;
                 return a->name < b->name;
             });

        return symbols;
    }

    void printConsole(const string& title) {
        cout << "\n" << string(100, '=') << "\n";
        cout << title << "\n";
        cout << string(100, '=') << "\n";

        auto symbols = getAllSymbols();

        if (symbols.empty()) {
            cout << "Symbol table is empty.\n";
            return;
        }

        cout << left << setw(20) << "Variable"
             << setw(15) << "Data Type"
             << setw(25) << "Value"
             << setw(10) << "Line"
             << setw(15) << "Initialized"
             << setw(15) << "Scope" << "\n";
        cout << string(100, '-') << "\n";

        for (auto* entry : symbols) {
            cout << left << setw(20) << entry->name
                 << setw(15) << entry->dataType;

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

            cout << setw(25) << valueStr
                 << setw(10) << entry->line_declared
                 << setw(15) << (entry->initialized ? "Yes" : "No")
                 << setw(15) << entry->scope << "\n";
        }
        cout << "\nTotal symbols: " << symbols.size() << "\n";
    }

    void saveToCSV(const string& filepath) {
        ofstream file(filepath);
        if (!file.is_open()) {
            cerr << "Error: Could not create CSV file: " << filepath << endl;
            return;
        }

        auto symbols = getAllSymbols();

        // Write CSV header
        file << "Variable,Data Type,Value,Line,Initialized,Scope\n";

        // Write each symbol
        for (auto* entry : symbols) {
            string valueStr;
            if (entry->initialized) {
                if (holds_alternative<int>(entry->value)) {
                    valueStr = to_string(get<int>(entry->value));
                } else if (holds_alternative<double>(entry->value)) {
                    valueStr = to_string(get<double>(entry->value));
                } else if (holds_alternative<string>(entry->value)) {
                    valueStr = "\"" + get<string>(entry->value) + "\"";
                } else if (holds_alternative<char>(entry->value)) {
                    valueStr = "'" + string(1, get<char>(entry->value)) + "'";
                } else {
                    valueStr = "(uninitialized)";
                }
            } else {
                valueStr = "(uninitialized)";
            }

            file << entry->name << ","
                 << entry->dataType << ","
                 << valueStr << ","
                 << entry->line_declared << ","
                 << (entry->initialized ? "Yes" : "No") << ","
                 << entry->scope << "\n";
        }

        file.close();
        cout << "Symbol table saved to CSV: " << filepath << endl;
    }
};

const set<string> SPEC_RESERVED = {
    "func", "class", "object", "member", "import", "exec",
    "for", "while", "if", "else", "in", "range", "return", "print",
    "vector", "push", "pop", "size", "len", "true", "false", "null"
};

string stripComment(const string& line) {
    size_t pos = line.find("//");
    if (pos == string::npos) return line;
    return line.substr(0, pos);
}

VarValue parseValue(const string& valueStr, const string& dataType) {
    string trimmed = valueStr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

    // Handle based on inferred data type
    if (dataType == "string") {
        return trimmed;
    }

    if (dataType == "char") {
        if (trimmed.length() >= 3 && trimmed.front() == '\'' && trimmed.back() == '\'') {
            return trimmed[1];
        }
    }

    if (dataType == "int") {
        try {
            return stoi(trimmed);
        } catch (...) {}
    }

    if (dataType == "float") {
        try {
            return stod(trimmed);
        } catch (...) {}
    }

    return monostate{};
}

// Parse lexical report to extract variable information with types
map<string, string> parseLexicalReport(const string& reportPath) {
    map<string, string> varTypes;  // variable_name -> data_type

    ifstream file(reportPath);
    if (!file.is_open()) {
        cerr << "Error: Could not open lexical report: " << reportPath << endl;
        return varTypes;
    }

    string line;
    bool inInferredTypes = false;

    while (getline(file, line)) {
        // Find the "Inferred Data Types:" section
        if (line.find("Inferred Data Types:") != string::npos) {
            inInferredTypes = true;
            continue;
        }

        // Stop when we reach the next section
        if (inInferredTypes && (line.find("Function Specializations:") != string::npos ||
                                line.find("All identifiers") != string::npos ||
                                line.empty())) {
            if (!line.empty() && line.find(":") == string::npos) {
                break;
            }
        }

        // Parse variable : type pairs
        if (inInferredTypes && line.find(":") != string::npos) {
            // Remove leading whitespace
            string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            size_t colonPos = trimmed.find(":");
            if (colonPos != string::npos) {
                string varName = trimmed.substr(0, colonPos);
                string varType = trimmed.substr(colonPos + 1);

                // Trim both
                varName.erase(varName.find_last_not_of(" \t") + 1);
                varType.erase(0, varType.find_first_not_of(" \t"));
                varType.erase(varType.find_last_not_of(" \t\r\n") + 1);

                if (!varName.empty() && !varType.empty()) {
                    varTypes[varName] = varType;
                }
            }
        }
    }

    file.close();
    return varTypes;
}

// Process source code to get values and build symbol table
void processSourceCode(const string& src, SymbolTable& symTable, const map<string, string>& varTypes) {
    regex RE_ASSIGNMENT(R"(([A-Za-z_]\w*)\s*=\s*(.+))");
    regex RE_VECTOR_DECL(R"(vector\s*<[^>]+>\s+([A-Za-z_]\w*))");

    stringstream ss(src);
    string line;
    int lineNum = 0;
    string currentScope = "global";

    // Track multi-line assignments
    string pending_var = "";
    string pending_value = "";
    bool in_multiline = false;
    int brace_count = 0;

    while (getline(ss, line)) {
        lineNum++;
        string cleaned = stripComment(line);

        if (cleaned.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // Track scope changes
        if (cleaned.find("func(") != string::npos || cleaned.find("func()") != string::npos) {
            size_t start = cleaned.find("'");
            size_t end = cleaned.rfind("'");
            if (start != string::npos && end != string::npos && start < end) {
                currentScope = cleaned.substr(start + 1, end - start - 1);
            }
        }

        if (cleaned.find("class(") != string::npos || cleaned.find("class()") != string::npos) {
            size_t start = cleaned.find("\"");
            size_t end = cleaned.rfind("\"");
            if (start != string::npos && end != string::npos && start < end) {
                currentScope = cleaned.substr(start + 1, end - start - 1);
            }
        }

        // Handle multi-line assignments
        if (in_multiline) {
            pending_value += " " + cleaned;

            for (char c : cleaned) {
                if (c == '[' || c == '{') brace_count++;
                if (c == ']' || c == '}') brace_count--;
            }

            if (brace_count == 0) {
                in_multiline = false;

                auto typeIt = varTypes.find(pending_var);
                string dataType = (typeIt != varTypes.end()) ? typeIt->second : "unknown";

                symTable.insert(pending_var, dataType, lineNum, currentScope);
                VarValue val = parseValue(pending_value, dataType);
                symTable.updateValue(pending_var, val, currentScope);

                pending_var = "";
                pending_value = "";
            }
            continue;
        }

        // Check for vector declarations
        smatch vec_match;
        if (regex_search(cleaned, vec_match, RE_VECTOR_DECL)) {
            string varName = vec_match[1];

            auto typeIt = varTypes.find(varName);
            string dataType = (typeIt != varTypes.end()) ? typeIt->second : "vector";

            symTable.insert(varName, dataType, lineNum, currentScope);
            continue;
        }

        // Check for assignments
        smatch match;
        if (regex_search(cleaned, match, RE_ASSIGNMENT)) {
            string varName = match[1];
            string valueStr = match[2];

            if (SPEC_RESERVED.count(varName)) continue;

            // Check for multi-line
            brace_count = 0;
            for (char c : valueStr) {
                if (c == '[' || c == '{') brace_count++;
                if (c == ']' || c == '}') brace_count--;
            }

            if (brace_count > 0) {
                in_multiline = true;
                pending_var = varName;
                pending_value = valueStr;
                continue;
            }

            // Get data type from lexical report
            auto typeIt = varTypes.find(varName);
            string dataType = (typeIt != varTypes.end()) ? typeIt->second : "unknown";

            // Insert into symbol table
            symTable.insert(varName, dataType, lineNum, currentScope);

            // Parse and store value
            VarValue val = parseValue(valueStr, dataType);
            symTable.updateValue(varName, val, currentScope);
        }
    }
}

int main(int argc, char* argv[]) {
    string lexicalReportPath = "../lexicalAnalyzer/lexical_report.txt";
    string sourceCodePath = "../SampleCode.txt";

    if (argc >= 2) {
        lexicalReportPath = argv[1];
    }
    if (argc >= 3) {
        sourceCodePath = argv[2];
    }

    cout << "Reading lexical report: " << lexicalReportPath << endl;
    cout << "Reading source code: " << sourceCodePath << endl << endl;

    // Parse lexical report to get variable types
    map<string, string> varTypes = parseLexicalReport(lexicalReportPath);

    cout << "Found " << varTypes.size() << " variables with inferred types from lexical report.\n\n";

    // Read source code
    ifstream sourceFile(sourceCodePath);
    if (!sourceFile.is_open()) {
        cerr << "Error: Could not open source file: " << sourceCodePath << endl;
        return 1;
    }

    stringstream buffer;
    buffer << sourceFile.rdbuf();
    sourceFile.close();
    string src = buffer.str();

    // Build symbol table
    SymbolTable symTable;
    processSourceCode(src, symTable, varTypes);

    // Print to console
    symTable.printConsole("C-ACCEL SYMBOL TABLE");

    // Save to CSV
    fs::path current = fs::current_path();
    fs::path project_root = current.parent_path();
    fs::path csv_path = project_root / "symbolTable" / "symbol_table.csv";
    fs::path txt_path = project_root / "symbolTable" / "symbol_table_report.txt";

    symTable.saveToCSV(csv_path.string());

    // Also save text report
    streambuf* coutbuf = cout.rdbuf();
    ofstream output_file(txt_path);

    if (output_file.is_open()) {
        cout.rdbuf(output_file.rdbuf());
        symTable.printConsole("C-ACCEL SYMBOL TABLE");
        cout.rdbuf(coutbuf);
        output_file.close();
        cout << "Text report saved to: " << txt_path << endl;
    }

    return 0;
}