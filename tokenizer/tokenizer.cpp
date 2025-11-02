#include "tokenizer.hpp"
#include <iostream>
#include <sstream>
#include <regex>
#include <map>
#include <iomanip>

using namespace std;

static inline string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Main tokenizer that maps to the lexical report structure
vector<Token> tokenizeLexicalReport(const string& reportText) {
    vector<Token> tokens;
    stringstream ss(reportText);
    string line;
    string currentSection;

    regex headerRe(R"(^([A-Za-z ].*?):)");        // captures section headers like "Literals:" or "Operators used..."
    regex kvPairRe(R"(^\s*([^\s:]+)\s*[:=]\s*(\d+))"); // matches "x : 4"
    regex listRe(R"(^\s*([A-Za-z0-9_\"']+))");   // matches comma-separated names

    while (getline(ss, line)) {
        string raw = trim(line);
        if (raw.empty()) continue;

        smatch m;
        // detect section header
        if (regex_search(raw, m, headerRe)) {
            currentSection = trim(m[1]);
            continue;
        }

        if (currentSection.empty()) continue;

        // Based on section, apply different parsing
        if (currentSection.find("Literals") != string::npos) {
            // Parse literals
            regex literalRe(R"(".*?"|'.*?'|\b\d+(\.\d+)?\b)");
            for (sregex_iterator it(raw.begin(), raw.end(), literalRe), end; it != end; ++it)
                tokens.push_back({"Literal", it->str(), 1});
        }
        else if (currentSection.find("Operators") != string::npos) {
            if (regex_search(raw, m, kvPairRe))
                tokens.push_back({"Operator", m[1], stoi(m[2])});
        }
        else if (currentSection.find("Reserved") != string::npos) {
            if (regex_search(raw, m, kvPairRe))
                tokens.push_back({"ReservedWord", m[1], stoi(m[2])});
        }
        else if (currentSection.find("Data types") != string::npos) {
            if (regex_search(raw, m, kvPairRe))
                tokens.push_back({"DataType", m[1], stoi(m[2])});
        }
        else if (currentSection.find("Variables declared") != string::npos) {
            // Collect all identifiers separated by commas
            stringstream varStream(raw);
            string var;
            while (getline(varStream, var, ',')) {
                string cleaned = trim(var);
                if (!cleaned.empty())
                    tokens.push_back({"Variable", cleaned, 1});
            }
        }
        else if (currentSection.find("Duplicate") != string::npos) {
            if (raw.find("(none)") != string::npos) continue;
            stringstream dupStream(raw);
            string item;
            while (getline(dupStream, item, ',')) {
                string cleaned = trim(item);
                if (!cleaned.empty())
                    tokens.push_back({"DuplicateVariable", cleaned, 1});
            }
        }
        else if (currentSection.find("All identifiers") != string::npos) {
            stringstream idStream(raw);
            string item;
            while (getline(idStream, item, ',')) {
                string cleaned = trim(item);
                if (!cleaned.empty())
                    tokens.push_back({"Identifier", cleaned, 1});
            }
        }
    }

    return tokens;
}

// Print formatted summary
void printTokenSummary(const vector<Token>& tokens) {
    if (tokens.empty()) {
        cout << "No tokens parsed from lexical report.\n";
        return;
    }

    cout << "\n====================[ TOKEN SUMMARY ]====================\n";
    map<string, int> categoryCounts;

    for (const auto& t : tokens)
        categoryCounts[t.category] += t.count;

    for (const auto& [cat, cnt] : categoryCounts)
        cout << setw(18) << left << cat << " : " << cnt << "\n";

    cout << "=========================================================\n\n";

    // Optional detailed list
    for (const auto& t : tokens)
        cout << "[" << setw(14) << left << t.category << "] "
             << setw(15) << left << t.value
             << " (" << t.count << ")\n";

    cout << "\nTotal tokens parsed: " << tokens.size() << endl;
}
