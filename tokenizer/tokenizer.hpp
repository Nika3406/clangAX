#pragma once
#include <string>
#include <vector>
#include <map>

struct Token {
    std::string category; // e.g. "Literal", "Operator"
    std::string value;    // e.g. "+", "45", "func"
    int count = 1;        // if applicable (for counted categories)
};

// Parse and tokenize a lexical_report.txt into categorized tokens
std::vector<Token> tokenizeLexicalReport(const std::string& reportText);

// Print token breakdown summary
void printTokenSummary(const std::vector<Token>& tokens);
