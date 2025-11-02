#include "tokenizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

int main() {
    string path = "../lexicalAnalyzer/lexical_report.txt";
    cout << "Working directory: " << fs::current_path() << endl;
    cout << "Attempting to open: " << path << endl;

    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Could not open " << path << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string reportText = buffer.str();
    file.close();

    cout << "\nSuccessfully loaded lexical_report.txt ("
         << reportText.size() << " bytes)\n";

    auto tokens = tokenizeLexicalReport(reportText);
    printTokenSummary(tokens);

    // Save tokens to file
    string outPath = "../tokenizer/tokens_output.txt";
    ofstream out(outPath);
    if (out.is_open()) {
        for (const auto& t : tokens)
            out << "[" << t.category << "] " << t.value << " (" << t.count << ")\n";
        out.close();
        cout << "\nTokens saved to: " << outPath << endl;
    }

    return 0;
}
