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
#include <stack>

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
    string scope;

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
        int index = hashFunction(name + scope);  // Hash with scope for uniqueness

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
        int index = hashFunction(name + scope);

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
        int index = hashFunction(name + scope);

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
                    double val = get<double>(entry->value);
                    char buffer[50];
                    snprintf(buffer, sizeof(buffer), "%.6f", val);
                    valueStr = buffer;
                } else if (holds_alternative<string>(entry->value)) {
                    string val = get<string>(entry->value);
                    // Check if it's a special marker
                    if (val == "[array]" || val == "[vector]") {
                        valueStr = val;
                    } else if (val.length() > 20) {
                        valueStr = "\"" + val.substr(0, 17) + "...\"";
                    } else {
                        valueStr = "\"" + val + "\"";
                    }
                } else if (holds_alternative<char>(entry->value)) {
                    valueStr = "'" + string(1, get<char>(entry->value)) + "'";
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

        file << "Variable,Data Type,Value,Line,Initialized,Scope\n";

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

    if (dataType == "string") {
        // Remove quotes if present
        if (trimmed.length() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
            return trimmed.substr(1, trimmed.length() - 2);
        }
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

    if (dataType == "float" || dataType == "double") {
        try {
            return stod(trimmed);
        } catch (...) {}
    }

    // For arrays, just return a placeholder string
    if (dataType == "array") {
        if (trimmed.find('[') != string::npos) {
            return string("[array]");
        }
    }

    // For vectors, return placeholder
    if (dataType == "vector") {
        return string("[vector]");
    }

    // For identifiers/function calls, return the reference
    if (dataType == "identifier" || dataType == "function_call") {
        return trimmed;
    }

    return monostate{};
}

map<string, string> parseLexicalReport(const string& reportPath) {
    map<string, string> varTypes;

    ifstream file(reportPath);
    if (!file.is_open()) {
        cerr << "Error: Could not open lexical report: " << reportPath << endl;
        return varTypes;
    }

    string line;
    bool inInferredTypes = false;

    while (getline(file, line)) {
        if (line.find("Inferred Data Types:") != string::npos) {
            inInferredTypes = true;
            continue;
        }

        if (inInferredTypes && (line.find("Function Specializations:") != string::npos ||
                                line.find("All identifiers") != string::npos)) {
            break;
        }

        if (inInferredTypes && line.find(":") != string::npos) {
            string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            size_t colonPos = trimmed.find(":");
            if (colonPos != string::npos) {
                string varName = trimmed.substr(0, colonPos);
                string varType = trimmed.substr(colonPos + 1);

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

void processSourceCode(const string& src, SymbolTable& symTable, const map<string, string>& varTypes) {
    regex RE_ASSIGNMENT(R"(([A-Za-z_]\w*)\s*=\s*(.+))");
    regex RE_VECTOR_DECL(R"(vector\s*<[^>]+>\s+([A-Za-z_]\w*))");
    regex RE_FUNC_START(R"(func\([^)]*\)\s*=\s*["\']([^"\']+)["\'])");
    regex RE_CLASS_START(R"(class\([^)]*\)\s*=\s*["\']([^"\']+)["\'])");

    stringstream ss(src);
    string line;
    int lineNum = 0;

    int braceDepth = 0;  // Track overall brace depth
    map<int, string> depthToScope;  // Map brace depth to scope name
    depthToScope[0] = "global";

    string currentClass = "";  // Track current class context
    int classStartDepth = -1;  // Track the brace depth where class started

    // Multi-line tracking
    string pending_var = "";
    string pending_value = "";
    bool in_multiline = false;
    int multiline_brace_count = 0;

    while (getline(ss, line)) {
        lineNum++;
        string cleaned = stripComment(line);

        if (cleaned.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // Track braces for scope management
        int openBraces = 0, closeBraces = 0;
        for (char c : cleaned) {
            if (c == '{') openBraces++;
            else if (c == '}') closeBraces++;
        }

        // Detect class scope BEFORE updating brace depth
        smatch class_match;
        if (regex_search(cleaned, class_match, RE_CLASS_START)) {
            string className = class_match[1];
            currentClass = className;
            classStartDepth = braceDepth + openBraces - closeBraces;
            depthToScope[classStartDepth] = className;
        }

        // Update brace depth
        braceDepth += (openBraces - closeBraces);

        // Reset class context when we exit the class's brace level
        if (classStartDepth >= 0 && braceDepth <= classStartDepth) {
            if (closeBraces > 0) {  // Only reset if we actually closed a brace
                currentClass = "";
                classStartDepth = -1;
            }
        }

        // Clean up scope map for closed braces
        if (closeBraces > 0) {
            for (int d = braceDepth + 1; d <= braceDepth + closeBraces; d++) {
                if (depthToScope.count(d)) {
                    depthToScope.erase(d);
                }
            }
        }

        // Detect function scope
        smatch func_match;
        if (regex_search(cleaned, func_match, RE_FUNC_START)) {
            string funcName = func_match[1];
            // If we're in a class (currentClass is set), prefix function name
            if (!currentClass.empty()) {
                depthToScope[braceDepth] = currentClass + "::" + funcName;
            } else {
                depthToScope[braceDepth] = funcName;
            }
        }

        // Get current scope based on brace depth
        string currentScope = "global";
        for (int d = braceDepth; d >= 0; d--) {
            if (depthToScope.count(d)) {
                currentScope = depthToScope[d];
                break;
            }
        }

        // Handle multi-line assignments
        if (in_multiline) {
            pending_value += " " + cleaned;

            for (char c : cleaned) {
                if (c == '[' || c == '{') multiline_brace_count++;
                if (c == ']' || c == '}') multiline_brace_count--;
            }

            if (multiline_brace_count == 0) {
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
            multiline_brace_count = 0;
            for (char c : valueStr) {
                if (c == '[' || c == '{') multiline_brace_count++;
                if (c == ']' || c == '}') multiline_brace_count--;
            }

            if (multiline_brace_count > 0) {
                in_multiline = true;
                pending_var = varName;
                pending_value = valueStr;
                continue;
            }

            auto typeIt = varTypes.find(varName);
            string dataType = (typeIt != varTypes.end()) ? typeIt->second : "unknown";

            symTable.insert(varName, dataType, lineNum, currentScope);

            VarValue val = parseValue(valueStr, dataType);
            symTable.updateValue(varName, val, currentScope);
        }
    }
}

int main(int argc, char* argv[]) {
    string lexicalReportPath = "../lexicalAnalyzer/lexical_report.txt";
    string sourceCodePath = "../SampleCode.cax";

    if (argc >= 2) {
        lexicalReportPath = argv[1];
    }
    if (argc >= 3) {
        sourceCodePath = argv[2];
    }

    cout << "Reading lexical report: " << lexicalReportPath << endl;
    cout << "Reading source code: " << sourceCodePath << endl << endl;

    map<string, string> varTypes = parseLexicalReport(lexicalReportPath);

    cout << "Found " << varTypes.size() << " variables with inferred types from lexical report.\n\n";

    ifstream sourceFile(sourceCodePath);
    if (!sourceFile.is_open()) {
        cerr << "Error: Could not open source file: " << sourceCodePath << endl;
        return 1;
    }

    stringstream buffer;
    buffer << sourceFile.rdbuf();
    sourceFile.close();
    string src = buffer.str();

    SymbolTable symTable;
    processSourceCode(src, symTable, varTypes);

    symTable.printConsole("C-ACCEL SYMBOL TABLE");

    string output_path = "../symbol_table_report.txt";
    ofstream output_file(output_path);
    if (output_file.is_open()) {
        streambuf* coutbuf = cout.rdbuf();
        cout.rdbuf(output_file.rdbuf());
        symTable.printConsole("C-ACCEL SYMBOL TABLE");
        cout.rdbuf(coutbuf);
        output_file.close();
        cout << "\nText report saved to: " << output_path << endl;
    }

    string csv_path = "../symbol_table.csv";
    symTable.saveToCSV(csv_path);
    cout << "CSV saved to: " << csv_path << endl;

    return 0;
}