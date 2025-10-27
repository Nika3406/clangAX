#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <algorithm>
#include <sstream>

using namespace std;

const set<string> SPEC_RESERVED = {
    "func", "class", "object", "member", "import", "exec",
    "for", "while", "if", "else", "in", "range", "return", "print"
};

const set<string> SPEC_DATATYPES = {
    "int", "float", "char", "string", "array", "matrix", "object", "dataset", "neuro"
};

const vector<string> SPEC_OPERATORS = {"==", "++", "--", "+", "-", "*", "/", "=", "<", ">"};

struct LexicalReport {
    int lines_processed;
    int literals_total_count;
    vector<string> literals_unique;
    map<string, int> operators_counts;
    map<string, int> reserved_words_counts;
    map<string, int> data_types_used_counts;
    vector<string> variables_declared;
    vector<string> variables_all_identifiers_seen;
    vector<string> duplicate_declarations;
};

string stripComment(const string& line) {
    size_t pos = line.find('#');
    if (pos == string::npos) {
        return line;
    }
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
            if (code.substr(i, op.length()) == op) {
                counts[op]++;
                i += op.length();
                matched = true;
                break;
            }
        }
        if (!matched) {
            i++;
        }
    }
    return counts;
}

LexicalReport tokenizeAndAnalyze(const string& src) {
    LexicalReport report;
    report.lines_processed = 0;
    report.literals_total_count = 0;
    
    vector<string> literals;
    set<string> declared_vars;
    map<string, int> declaration_counts;
    set<string> duplicate_declarations;
    set<string> variables_used;
    bool in_object_block = false;
    
    regex RE_STRING(R"("([^"\\]|\\.)*")");
    regex RE_CHAR(R"('([^'\\]|\\.)')");
    regex RE_FLOAT(R"((?:^|[^\w.])(?:\d+\.\d+|\d+\.(?!\w)|\.\d+)(?:[^\w.]|$))");
    regex RE_INT(R"((?:^|[^\w.])\d+(?:[^\w.]|$))");
    regex RE_IDENT(R"([A-Za-z_]\w*)");
    
    stringstream ss(src);
    string raw;
    
    while (getline(ss, raw)) {
        string no_comment = stripComment(raw);
        if (no_comment.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }
        report.lines_processed++;
        
        string line = no_comment;
        
        // Find string literals
        for (sregex_iterator it(line.begin(), line.end(), RE_STRING), end; it != end; ++it) {
            literals.push_back(it->str());
        }
        
        // Find char literals
        for (sregex_iterator it(line.begin(), line.end(), RE_CHAR), end; it != end; ++it) {
            literals.push_back(it->str());
        }
        
        // Mask out string and char literals
        string temp = line;
        for (sregex_iterator it(line.begin(), line.end(), RE_STRING), end; it != end; ++it) {
            size_t start = it->position();
            size_t len = it->length();
            temp.replace(start, len, string(len, ' '));
        }
        for (sregex_iterator it(line.begin(), line.end(), RE_CHAR), end; it != end; ++it) {
            size_t start = it->position();
            size_t len = it->length();
            temp.replace(start, len, string(len, ' '));
        }
        
        // Find float literals
        for (sregex_iterator it(temp.begin(), temp.end(), RE_FLOAT), end; it != end; ++it) {
            string match = it->str();
            match.erase(0, match.find_first_not_of(" \t"));
            match.erase(match.find_last_not_of(" \t") + 1);
            literals.push_back(match);
        }
        
        // Mask floats and find integers
        string temp2 = temp;
        temp2 = regex_replace(temp2, RE_FLOAT, " ");
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_INT), end; it != end; ++it) {
            string match = it->str();
            match.erase(0, match.find_first_not_of(" \t"));
            match.erase(match.find_last_not_of(" \t") + 1);
            literals.push_back(match);
        }
        
        // Count operators
        auto op_counts = findOperators(temp2);
        for (const auto& [op, count] : op_counts) {
            report.operators_counts[op] += count;
        }
        
        // Count reserved words
        for (const auto& kw : SPEC_RESERVED) {
            regex kw_regex("\\b" + kw + "\\b");
            for (sregex_iterator it(temp2.begin(), temp2.end(), kw_regex), end; it != end; ++it) {
                report.reserved_words_counts[kw]++;
            }
        }
        
        // Check for object block
        if (regex_search(temp2, regex(R"(\bobject\s*:\b)"))) {
            in_object_block = true;
        }
        
        // Handle object block declarations
        if (in_object_block) {
            if (regex_search(temp2, regex(R"(\bmember\s*:\b)"))) {
                in_object_block = false;
            } else {
                for (sregex_iterator it(temp2.begin(), temp2.end(), RE_IDENT), end; it != end; ++it) {
                    string name = it->str();
                    if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name) || name == "object") {
                        continue;
                    }
                    if (declared_vars.count(name)) {
                        duplicate_declarations.insert(name);
                    }
                    declared_vars.insert(name);
                    declaration_counts[name]++;
                    variables_used.insert(name);
                }
            }
        }
        
        // Find typed declarations
        string types_pattern = "(";
        bool first = true;
        for (const auto& t : SPEC_DATATYPES) {
            if (!first) types_pattern += "|";
            types_pattern += t;
            first = false;
        }
        types_pattern += R"()\s+([A-Za-z_]\w*)(?!\s*\())";
        regex RE_TYPED_DECL(types_pattern);
        
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_TYPED_DECL), end; it != end; ++it) {
            string type = (*it)[1];
            string name = (*it)[2];
            report.data_types_used_counts[type]++;
            if (declared_vars.count(name)) {
                duplicate_declarations.insert(name);
            }
            declared_vars.insert(name);
            declaration_counts[name]++;
            variables_used.insert(name);
        }
        
        // Find assignments
        regex RE_ASSIGN(R"(([A-Za-z_]\w*)\s*=\s*(?!=))");
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_ASSIGN), end; it != end; ++it) {
            string name = (*it)[1];
            if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name)) {
                continue;
            }
            if (!declared_vars.count(name)) {
                declared_vars.insert(name);
                declaration_counts[name]++;
            }
            variables_used.insert(name);
        }
        
        // Find all identifiers
        for (sregex_iterator it(temp2.begin(), temp2.end(), RE_IDENT), end; it != end; ++it) {
            string name = it->str();
            if (SPEC_RESERVED.count(name) || SPEC_DATATYPES.count(name)) {
                continue;
            }
            variables_used.insert(name);
        }
    }
    
    // Build unique literals list
    set<string> seen;
    for (const auto& lit : literals) {
        if (!seen.count(lit)) {
            seen.insert(lit);
            report.literals_unique.push_back(lit);
        }
    }
    
    report.literals_total_count = literals.size();
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
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <source.txt>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "File not found: " << filename << endl;
        return 2;
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    string src = buffer.str();
    file.close();
    
    LexicalReport rep = tokenizeAndAnalyze(src);
    string report = formatReport(rep);

    // Output to console
    cout << report << endl;

    // Output to file in the same directory as input file
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