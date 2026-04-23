#include "Python3Lexer.h"
#include "Python3Parser.h"
#include "antlr4-runtime.h"
#include <iostream>

using namespace antlr4;

int main() {
    try {
        std::string input = "print(123)\n";
        ANTLRInputStream antlrInput(input);
        Python3Lexer lexer(&antlrInput);
        CommonTokenStream tokens(&lexer);
        tokens.fill();

        std::cout << "Tokens:" << std::endl;
        for (auto token : tokens.getTokens()) {
            std::cout << token->toString() << std::endl;
        }

        Python3Parser parser(&tokens);
        parser.removeErrorListeners();
        tree::ParseTree *tree = parser.file_input();

        if (parser.getNumberOfSyntaxErrors() > 0) {
            std::cout << "Syntax errors found: " << parser.getNumberOfSyntaxErrors() << std::endl;
        } else {
            std::cout << "Parse successful!" << std::endl;
            std::cout << "Tree: " << tree->toStringTree(&parser) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}