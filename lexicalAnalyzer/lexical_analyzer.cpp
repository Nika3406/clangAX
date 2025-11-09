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
#include "symbol_table.h"

using namespace std;
namespace fs = std::filesystem;


const set<string> SPEC_RESERVED = {
    "func", "class", "object", "member", "import", "exec",
    "for", "while", "if", "else", "in", "range", "return", "print"
};

const set<string> SPEC_DATATYPES = {
    "int", "float", "char", "string", "array", "matrix", "object", "dataset", "neuro"
};

const vector<string> SPEC_OPERATORS = {"==", "++", "--", "+", "-", "*", "/", "=", "<", ">"};

struct LexicalReport {
    int lines_processed = 0;
    int literals_total_count = 0;
    vector<string> literals_unique;
    map<string, int> operators_counts;
    map<string, int> reserved_words_counts;
    map<string, int> data_types_used_counts;
    vector<string> variables_declared;
    vector<string> variables_all_identifiers_seen;
    vector<string> duplicate_declarations;
};

string stripComment(const string& line) {
    // Treat '//' as comment; keep lines beginning with '#' (e.g., #import).
    size_t pos = line.find("//");
    if (pos == string::npos) return line;
    return line.substr(0, pos);
}

map<string, int> findOperators(const string& code) {
    map<string, int> counts;
    vector<string> ordered = SPEC_OPERATORS;
    sort(ordered.begin(), ordered.end(), [](const string& a, const string& b) {
        return a.length() > b.length();
    });

    size_t i = 0;
    while (i < code.length()) {
        bool matched = false;
        for (const auto& op : ordered) {
            if (i + op.length() <= code.length() && code.substr(i, op.length()) == op) {
                counts[op]++;
                i += op.length();
                matched = true;
                break;
            }
        }
        if (!matched) i++;
    }
    return counts;
}

LexicalReport tokenizeAndAnalyze(const string& src) {
    LexicalReport report;

    vector<string> literals;
    set<string> declared_vars;
    map<string, int> declaration_counts;
    set<string> duplicate_declarations;
    set<string> variables_used;
    bool in_object_block = false;

    regex RE_STRING(R"("([^"\\]|\\.)*")");
    regex RE_CHAR(R"('([^'\\]|\\.)')");
    // simpler and safer number regexes
    regex RE_FLOAT(R"(\b\d+\.\d+\b|\b\d+\.\b|\b\.\d+\b)");
    regex RE_INT(R"(\b\d+\b)");
    regex RE_IDENT(R"([A-Za-z_]\w*)");

    // Build typed-declaration regex from SPEC_DATATYPES
    string types_joined;
    bool firstt = true;
    for (const auto& t : SPEC_DATATYPES) {
        if (!firstt) types_joined += "|";
        types_joined += regex_replace(t, regex(R"([\^$.|?*+()\\\[\]{}])"), "\\$&");
        firstt = false;
    }
    regex RE_TYPED_DECL(("\\b(" + types_joined + ")\\b\\s+([A-Za-z_]\\w*)(?!\\s*\\()"));

    stringstream ss(src);
    string raw;
    while (getline(ss, raw)) {
        string no_comment = stripComment(raw);
        // skip empty lines
        if (no_comment.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }
        report.lines_processed++;
        string line = no_comment;

        // Collect string and char literals
        for (sregex_iterator it(line.begin(), line.end(), RE_STRING), end; it != end; ++it) {
            literals.push_back(it->str());
        }
        for (sregex_iterator it(line.begin(), line.end(), RE_CHAR), end; it != end; ++it) {
            literals.push_back(it->str());
        }
        // Mask strings and chars with spaces to preserve positions
        string temp = line;
        for (sregex_iterator it(line.begin(), line.end(), RE_STRING), end; it != end; ++it) {
            size_t start = it->position();
            size_t len = it->length();
            if (start + len <= temp.size()) temp.replace(start, len, string(len, ' '));
        }
        for (sregex_iterator it(line.begin(), line.end(), RE_CHAR), end; it != end; ++it) {
            size_t start = it->position();
            size_t len = it->length();
            if (start + len <= temp.size()) temp.replace(start, len, string(len, ' '));
        }

        // Eliminate punctuation that sticks to numbers/idents (commas, parens)
        temp = regex_replace(temp, regex(R"([(),])"), " ");

        // Find floats and ints
        for (sregex_iterator it(temp.begin(), temp.end(), RE_FLOAT), end; it != end; ++it) {
            literals.push_back(it->str());
        }
        string temp2 = regex_replace(temp, RE_FLOAT, " ");
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_INT), end; it != end; ++it) {
            literals.push_back(it->str());
        }

        // Count operators
        auto op_counts = findOperators(temp2);
        for (const auto& [op, count] : op_counts) report.operators_counts[op] += count;

        // Count reserved words (word boundaries)
        for (const auto& kw : SPEC_RESERVED) {
            regex kw_regex("\\b" + kw + "\\b");
            for (sregex_iterator it(temp2.begin(), temp2.end(), kw_regex), end; it != end; ++it) {
                report.reserved_words_counts[kw]++;
            }
        }
        // Treat '#import' as import
        if (regex_search(line, regex(R"(^\s*#\s*import\b)"))) {
            report.reserved_words_counts["import"]++;
        }

        // Object block detection
        if (regex_search(temp2, regex(R"(\bobject\s*:)"))) {
            in_object_block = true;
        }
        if (in_object_block && !regex_search(temp2, regex(R"(\bmember\s*:)"))) {
            // collect identifiers on object block lines as declarations
            for (sregex_iterator it(temp2.begin(), temp2.end(), RE_IDENT), end; it != end; ++it) {
                string name = it->str();
                if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name) || name == "object" || name == "member") continue;
                if (declared_vars.count(name)) duplicate_declarations.insert(name);
                declared_vars.insert(name);
                declaration_counts[name]++;
                variables_used.insert(name);
            }
        }
        if (regex_search(temp2, regex(R"(\bmember\s*:)"))) {
            in_object_block = false;
        }

        // Typed declarations
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_TYPED_DECL), end; it != end; ++it) {
            string type = (*it)[1];
            string name = (*it)[2];
            report.data_types_used_counts[type]++;
            if (declared_vars.count(name)) duplicate_declarations.insert(name);
            declared_vars.insert(name);
            declaration_counts[name]++;
            variables_used.insert(name);
        }

        // Assignments (ident = ...)
        regex RE_ASSIGN(R"(([A-Za-z_]\w*)\s*=\s*(?!=))");
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_ASSIGN), end; it != end; ++it) {
            string name = (*it)[1];
            if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name)) continue;
            if (!declared_vars.count(name)) {
                declared_vars.insert(name);
                declaration_counts[name]++;
            }
            variables_used.insert(name);
        }

        // All identifiers seen
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_IDENT), end; it != end; ++it) {
            string name = it->str();
            if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name)) continue;
            variables_used.insert(name);
        }
    } // end lines loop

    // Build literals unique list
    set<string> seen;
    for (const auto& lit : literals) {
        if (!seen.count(lit)) {
            seen.insert(lit);
            report.literals_unique.push_back(lit);
        }
    }
    report.literals_total_count = (int)literals.size();
    report.variables_declared = vector<string>(declared_vars.begin(), declared_vars.end());
    sort(report.variables_declared.begin(), report.variables_declared.end());

    report.variables_all_identifiers_seen = vector<string>(variables_used.begin(), variables_used.end());
    sort(report.variables_all_identifiers_seen.begin(), report.variables_all_identifiers_seen.end());

    report.duplicate_declarations = vector<string>(duplicate_declarations.begin(), duplicate_declarations.end());
    sort(report.duplicate_declarations.begin(), report.duplicate_declarations.end());

    return report;
}

string formatReport(const LexicalReport& rep) {
    stringstream ss;
    ss << "C-Accel Lexical Report\n";
    ss << "==========================\n";
    ss << "Lines processed (comments omitted): " << rep.lines_processed << "\n\n";

    ss << "Literals: total=" << rep.literals_total_count << "\n";
    if (!rep.literals_unique.empty()) {
        ss << "  Unique literals: ";
        for (size_t i = 0; i < rep.literals_unique.size(); i++) {
            if (i > 0) ss << ", ";
            ss << rep.literals_unique[i];
        }
        ss << "\n";
    } else {
        ss << "  Unique literals: (none)\n";
    }
    ss << "\n";

    if (!rep.operators_counts.empty()) {
        ss << "Operators used (with counts):\n";
        for (const auto& [op, cnt] : rep.operators_counts) {
            if (cnt > 0) {
                ss << "  " << setw(2) << op << " : " << cnt << "\n";
            }
        }
    } else {
        ss << "Operators used: (none)\n";
    }
    ss << "\n";

    if (!rep.reserved_words_counts.empty()) {
        ss << "Reserved words used (with counts):\n";
        for (const auto& [kw, cnt] : rep.reserved_words_counts) {
            if (cnt > 0) {
                ss << "  " << kw << ": " << cnt << "\n";
            }
        }
    } else {
        ss << "Reserved words used: (none)\n";
    }
    ss << "\n";

    if (!rep.data_types_used_counts.empty()) {
        ss << "Data types used in declarations (with counts):\n";
        for (const auto& [t, cnt] : rep.data_types_used_counts) {
            ss << "  " << t << ": " << cnt << "\n";
        }
    } else {
        ss << "Data types used in declarations: (none)\n";
    }
    ss << "\n";

    if (!rep.variables_declared.empty()) {
        ss << "Variables declared (" << rep.variables_declared.size() << "):\n  ";
        for (size_t i = 0; i < rep.variables_declared.size(); i++) {
            if (i > 0) ss << ", ";
            ss << rep.variables_declared[i];
        }
        ss << "\n";
    } else {
        ss << "Variables declared: (none)\n";
    }

    if (!rep.duplicate_declarations.empty()) {
        ss << "Duplicate declarations detected for: ";
        for (size_t i = 0; i < rep.duplicate_declarations.size(); i++) {
            if (i > 0) ss << ", ";
            ss << rep.duplicate_declarations[i];
        }
        ss << "\n";
    } else {
        ss << "Duplicate declarations detected: (none)\n";
    }
    ss << "\n";

    if (!rep.variables_all_identifiers_seen.empty()) {
        ss << "All identifiers seen (" << rep.variables_all_identifiers_seen.size() << "):\n  ";
        for (size_t i = 0; i < rep.variables_all_identifiers_seen.size(); i++) {
            if (i > 0) ss << ", ";
            ss << rep.variables_all_identifiers_seen[i];
        }
        ss << "\n";
    }

    return ss.str();
}

int main(int argc, char* argv[]) {
    string filename;

    if (argc < 2) {
        // Default to SampleCode.txt located in the parent folder
        filename = "../SampleCode.txt";
        cout << "No source file provided. Using default: " << filename << endl;
    } else {
        filename = argv[1];
    }

    ifstream file(filename);

    // If not found, also try "../" relative path
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

     LexicalReport rep = tokenizeAndAnalyze(src);
    string report = formatReport(rep);

    // Output lexical report to console and file (unchanged)
    cout << report << endl;

    // Build symbol table from lexical report:
    SymbolTable symtab;

    // Populate: use declared variables and any identifier seen to create entries.
    // We prefer declared variables and attempt to attach types from data_types_used_counts where possible.
    // Since the lexical pass doesn't attach per-variable types, we'll insert declared names with unknown type.
    for (const auto& name : rep.variables_declared) {
        symtab.insert(name, "", "(unset)", -1);
    }
    // Ensure all identifiers seen exist in table (without overwriting declared ones)
    for (const auto& name : rep.variables_all_identifiers_seen) {
        symtab.upsert(name, "", "", -1);
    }

    // Print initial state of symbol table
    cout << "\n--- Initial Symbol Table ---\n";
    cout << symtab.formatTable() << endl;

    // Perform example updates: set some values for a subset of declared variables.
    // Use a deterministic selection: first three symbols (alphabetical) will be updated.
    auto symbols = symtab.allSymbols();
    for (size_t i = 0; i < symbols.size() && i < 3; ++i) {
        const auto& s = symbols[i];
        // Example assignments (toy): for naming clarity, set value to "val_<name>" or numeric for likely numeric names.
        string newval = "val_" + s.name;
        // very small heuristic: if name contains 'i' or 'n' maybe an int-like value
        if (s.name.find_first_of("0123456789") != string::npos || s.name.find('i') != string::npos) {
            newval = "0";
        }
        symtab.updateValue(s.name, newval);
    }

    // Also demonstrate type updates for up to two variables using types seen in lexical report.
    // Assign types in round-robin from data_types_used_counts keys (if any)
    vector<string> types;
    for (const auto& kv : rep.data_types_used_counts) types.push_back(kv.first);
    if (!types.empty()) {
        for (size_t i = 0; i < symbols.size() && i < 2; ++i) {
            symtab.updateType(symbols[i].name, types[i % types.size()]);
        }
    }

    // Print final state after updates
    cout << "\n--- Final Symbol Table (after updates) ---\n";
    cout << symtab.formatTable() << endl;

    // Save lexical report to file (same as original behavior)
    size_t last_slash = filename.find_last_of("/\\");
    string output_path;
    if (last_slash != string::npos) {
        output_path = filename.substr(0, last_slash + 1) + "lexical_report.txt";
    } else {
        output_path = "lexical_report.txt";
    }

    ofstream output_file(output_path);
    if (output_file.is_open()) {
        output_file << report;
        output_file.close();
        cout << "\nReport saved to: " << output_path << endl;
    } else {
        cout << "\nWarning: Could not write report to file" << endl;
    }

    return 0;
}
