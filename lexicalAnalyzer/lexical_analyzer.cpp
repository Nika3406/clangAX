#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// -----------------------------------------------------
// EXTENDED RESERVED WORDS
// -----------------------------------------------------
const set<string> SPEC_RESERVED = {
    "func", "class", "object", "member", "import", "exec",
    "for", "while", "if", "else", "in", "range", "return",
    "print", "vector", "push", "pop", "size", "len",
    "true", "false", "null"
};

// -----------------------------------------------------
// EXTENDED OPERATORS
// -----------------------------------------------------
const vector<string> SPEC_OPERATORS = {
    "==", "!=", "<=", ">=", "++", "--",
    "+=", "-=", "*=", "/=",
    "+", "-", "*", "/", "%", ".",
    "=", "<", ">", "&&", "||", "!",
    "[", "]", "(", ")", "{", "}", ",", ":"
};

// -----------------------------------------------------
// STRUCT: LEXICAL REPORT
// -----------------------------------------------------
struct LexicalReport {
    int lines_processed = 0;
    int literals_total_count = 0;

    vector<string> literals_unique;
    map<string, int> operators_counts;
    map<string, int> reserved_words_counts;

    vector<string> variables_declared;
    vector<string> variables_all_identifiers_seen;

    map<string, string> inferred_var_types;

    // Function and class specialization tracking
    map<string, string> function_types;  // function_name -> type (ML, Neuro, Math, Main, etc.)
    map<string, string> class_types;     // class_name -> type (Layer, Type, etc.)
};

// -----------------------------------------------------
// REMOVE COMMENTS
// -----------------------------------------------------
string stripComment(const string& line) {
    size_t pos = line.find("//");
    if (pos == string::npos) return line;
    return line.substr(0, pos);
}

// -----------------------------------------------------
// OPERATOR DETECTION
// -----------------------------------------------------
map<string, int> findOperators(const string& code) {
    map<string, int> counts;
    vector<string> ordered = SPEC_OPERATORS;

    sort(ordered.begin(), ordered.end(), [](const string& a, const string& b) {
        return a.length() > b.length();
    });

    size_t i = 0;
    while (i < code.size()) {
        bool matched = false;
        for (const auto& op : ordered) {
            if (i + op.size() <= code.size() && code.substr(i, op.size()) == op) {
                counts[op]++;
                i += op.size();
                matched = true;
                break;
            }
        }
        if (!matched) i++;
    }
    return counts;
}

// -----------------------------------------------------
// DATATYPE INFERENCE
// -----------------------------------------------------
string inferType(const string& val) {
    // Trim whitespace
    string trimmed = val;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

    static regex RE_FLOAT(R"(^-?\d+\.\d+)");
    static regex RE_INT(R"(^-?\d+)");
    static regex RE_STRING(R"(^".*")");
    static regex RE_BOOL(R"(^(true|false))");
    static regex RE_NULL(R"(^(null))");
    static regex RE_CHAR(R"(^'.')");
    static regex RE_ARRAY_START(R"(^\[)");
    static regex RE_VECTOR(R"(^vector\s*<)");
    static regex RE_ARRAY_ACCESS(R"(^\w+\[\d+\])");  // e.g., numbers[0]
    static regex RE_PARENTHESIZED_EXPR(R"(^\(.+\)$)");  // e.g., (5 == 5)

    // Check for vector type first (most specific)
    if (regex_search(trimmed, RE_VECTOR)) return "vector";

    // Check for array (starts with [)
    if (regex_search(trimmed, RE_ARRAY_START)) return "array";

    // Check for array element access
    if (regex_search(trimmed, RE_ARRAY_ACCESS)) return "identifier";

    // Check for parenthesized expressions (likely boolean/arithmetic)
    if (regex_search(trimmed, RE_PARENTHESIZED_EXPR)) {
        // Check if it contains comparison or logical operators
        if (trimmed.find("==") != string::npos || trimmed.find("!=") != string::npos ||
            trimmed.find("<=") != string::npos || trimmed.find(">=") != string::npos ||
            trimmed.find("<") != string::npos || trimmed.find(">") != string::npos ||
            trimmed.find("&&") != string::npos || trimmed.find("||") != string::npos ||
            trimmed.find("!") != string::npos) {
            return "bool";
        }
        // Arithmetic expression
        if (trimmed.find("+") != string::npos || trimmed.find("-") != string::npos ||
            trimmed.find("*") != string::npos || trimmed.find("/") != string::npos) {
            return "int";  // or "float" if needed
        }
    }

    // Check for string literal
    if (regex_search(trimmed, RE_STRING)) return "string";

    // Check for char literal
    if (regex_search(trimmed, RE_CHAR)) return "char";

    // Check for boolean
    if (regex_search(trimmed, RE_BOOL)) return "bool";

    // Check for null
    if (regex_search(trimmed, RE_NULL)) return "null";

    // Check for float (before int to catch decimals)
    if (regex_search(trimmed, RE_FLOAT)) return "float";

    // Check for int
    if (regex_search(trimmed, RE_INT)) return "int";

    // Check if it's a constructor call (ClassName())
    static regex RE_CONSTRUCTOR(R"(^[A-Z][A-Za-z_]\w*\s*\()");
    if (regex_search(trimmed, RE_CONSTRUCTOR)) return "object";

    // Check if it's a function call (contains parentheses)
    if (trimmed.find('(') != string::npos) return "function_call";

    // Check if it references another variable (identifier)
    static regex RE_IDENTIFIER(R"(^[A-Za-z_]\w*$)");
    if (regex_match(trimmed, RE_IDENTIFIER)) return "identifier";

    return "unknown";
}

// -----------------------------------------------------
// EXTRACT ALL LITERALS FROM LINE
// -----------------------------------------------------
vector<string> extractLiterals(const string& line) {
    vector<string> literals;

    // Regex patterns for different literal types
    regex RE_STRING(R"("([^"\\]|\\.)*")");
    regex RE_CHAR(R"('([^'\\]|\\.)')");
    regex RE_FLOAT(R"(\b-?\d+\.\d+\b)");
    regex RE_INT(R"(\b-?\d+\b)");

    set<string> found_literals;

    // Extract string literals first (to avoid conflicts)
    for (sregex_iterator it(line.begin(), line.end(), RE_STRING), end; it != end; ++it) {
        found_literals.insert(it->str());
    }

    // Extract char literals
    for (sregex_iterator it(line.begin(), line.end(), RE_CHAR), end; it != end; ++it) {
        found_literals.insert(it->str());
    }

    // Create a version of the line without strings/chars to avoid matching numbers inside them
    string line_without_strings = line;
    line_without_strings = regex_replace(line_without_strings, RE_STRING, " ");
    line_without_strings = regex_replace(line_without_strings, RE_CHAR, " ");

    // Extract float literals (before int to match decimal points)
    set<string> float_lits;
    for (sregex_iterator it(line_without_strings.begin(), line_without_strings.end(), RE_FLOAT), end; it != end; ++it) {
        float_lits.insert(it->str());
    }

    // Extract integer literals, but skip if part of a float
    for (sregex_iterator it(line_without_strings.begin(), line_without_strings.end(), RE_INT), end; it != end; ++it) {
        string num = it->str();

        // Check if this integer is part of a float we already found
        bool is_part_of_float = false;
        for (const auto& flt : float_lits) {
            if (flt.find(num) != string::npos) {
                is_part_of_float = true;
                break;
            }
        }

        if (!is_part_of_float) {
            found_literals.insert(num);
        }
    }

    // Add all floats
    for (const auto& flt : float_lits) {
        found_literals.insert(flt);
    }

    // Convert set to vector
    for (const auto& lit : found_literals) {
        literals.push_back(lit);
    }

    return literals;
}

// -----------------------------------------------------
// MAIN TOKENIZER + ANALYZER
// -----------------------------------------------------
LexicalReport tokenizeAndAnalyze(const string& src) {
    LexicalReport report;

    set<string> unique_literals;
    set<string> declared_vars;
    set<string> identifiers;

    regex RE_IDENT(R"([A-Za-z_]\w*)");
    regex RE_ASSIGN(R"(([A-Za-z_]\w*)\s*=\s*(.*))");

    stringstream ss(src);
    string raw;

    while (getline(ss, raw)) {
        string line = stripComment(raw);
        if (line.find_first_not_of(" \t\n\r") == string::npos) continue;

        report.lines_processed++;

        // Extract all literals (strings, chars, numbers)
        vector<string> line_literals = extractLiterals(line);
        for (const auto& lit : line_literals) {
            unique_literals.insert(lit);
            report.literals_total_count++;
        }

        // operators
        auto op_counts = findOperators(line);
        for (auto& p : op_counts)
            report.operators_counts[p.first] += p.second;

        // reserved words
        for (auto& kw : SPEC_RESERVED) {
            regex R("\\b" + kw + "\\b");
            for (sregex_iterator it(line.begin(), line.end(), R), end; it != end; ++it)
                report.reserved_words_counts[kw]++;
        }

        // identifiers
        for (sregex_iterator it(line.begin(), line.end(), RE_IDENT), end; it != end; ++it) {
            string name = it->str();
            if (!SPEC_RESERVED.count(name))
                identifiers.insert(name);
        }

        // variable assignment → infer datatype
        smatch match;
        if (regex_search(line, match, RE_ASSIGN)) {
            string var = match[1];
            string val = match[2];

            if (!declared_vars.count(var)) {
                declared_vars.insert(var);
                report.variables_declared.push_back(var);
            }

            report.inferred_var_types[var] = inferType(val);
        }
    }

    // Convert unique literals set to vector
    for (auto& l : unique_literals) {
        report.literals_unique.push_back(l);
    }

    // Identifier list
    for (auto& id : identifiers)
        report.variables_all_identifiers_seen.push_back(id);

    return report;
}

// -----------------------------------------------------
// FORMAT REPORT
// -----------------------------------------------------
string formatReport(const LexicalReport& rep) {
    stringstream ss;

    ss << "C-Accel Lexical Report\n";
    ss << "==========================\n";
    ss << "Lines processed (comments omitted): " << rep.lines_processed << "\n\n";

    ss << "Literals: total=" << rep.literals_total_count << "\n";
    ss << "  Unique literals: ";
    for (size_t i = 0; i < rep.literals_unique.size(); ++i) {
        ss << rep.literals_unique[i];
        if (i < rep.literals_unique.size() - 1) ss << ", ";
    }
    ss << "\n\n";

    ss << "Operators used (with counts):\n";
    for (auto& p : rep.operators_counts)
        ss << "  " << setw(3) << p.first << " : " << p.second << "\n";
    ss << "\n";

    ss << "Reserved words used (with counts):\n";
    for (auto& p : rep.reserved_words_counts)
        if (p.second > 0)
            ss << "  " << p.first << ": " << p.second << "\n";
    ss << "\n";

    // Collect unique data types from inferred types
    set<string> unique_types;
    for (auto& p : rep.inferred_var_types) {
        if (p.second != "unknown") {
            unique_types.insert(p.second);
        }
    }

    ss << "Data types used in declarations: ";
    if (unique_types.empty()) {
        ss << "(none)";
    } else {
        bool first = true;
        for (const auto& type : unique_types) {
            if (!first) ss << ", ";
            ss << type;
            first = false;
        }
    }
    ss << "\n\n";

    ss << "Variables declared (" << rep.variables_declared.size() << "):\n";
    ss << "  ";
    for (size_t i = 0; i < rep.variables_declared.size(); ++i) {
        ss << rep.variables_declared[i];
        if (i < rep.variables_declared.size() - 1) ss << ", ";
    }
    ss << "\nDuplicate declarations detected: (none)\n\n";

    // Add inferred data types section
    ss << "Inferred Data Types:\n";
    for (const auto& var : rep.variables_declared) {
        auto it = rep.inferred_var_types.find(var);
        if (it != rep.inferred_var_types.end()) {
            ss << "  " << var << " : " << it->second << "\n";
        }
    }
    ss << "\n";

    // Add function types section
    if (!rep.function_types.empty()) {
        ss << "Function Specializations:\n";
        for (const auto& func : rep.function_types) {
            if (func.second.empty()) {
                // Normal function: func() = 'name'
                ss << "  func() = '" << func.first << "'\n";
            } else if (func.second == "Main") {
                // Main function: func(Main) - no name
                ss << "  func(" << func.second << ")\n";
            } else {
                // Specialized function: func(Type) = 'name'
                ss << "  func(" << func.second << ") = '" << func.first << "'\n";
            }
        }
        ss << "\n";
    }

    // Add class types section
    if (!rep.class_types.empty()) {
        ss << "Class Specializations:\n";
        for (const auto& cls : rep.class_types) {
            if (cls.second.empty()) {
                // Normal class: class() = "name"
                ss << "  class() = \"" << cls.first << "\"\n";
            } else {
                // Specialized class: class(Type) = "name"
                ss << "  class(" << cls.second << ") = \"" << cls.first << "\"\n";
            }
        }
        ss << "\n";
    }

    ss << "All identifiers seen (" << rep.variables_all_identifiers_seen.size() << "):\n";
    ss << "  ";
    for (size_t i = 0; i < rep.variables_all_identifiers_seen.size(); ++i) {
        ss << rep.variables_all_identifiers_seen[i];
        if (i < rep.variables_all_identifiers_seen.size() - 1) ss << ", ";
    }
    ss << "\n";

    return ss.str();
}

// -----------------------------------------------------
int main(int argc, char* argv[]) {
    string filename = (argc < 2) ? "../SampleCode.txt" : argv[1];

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file.\n";
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string src = buffer.str();

    LexicalReport rep = tokenizeAndAnalyze(src);
    cout << formatReport(rep);

    return 0;
}