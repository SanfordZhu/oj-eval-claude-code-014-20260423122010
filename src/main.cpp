#include "Evalvisitor.h"
#include "Python3Lexer.h"
#include "Python3Parser.h"
#include "antlr4-runtime.h"
#include <iostream>
using namespace antlr4;
// TODO: regenerating files in directory named "generated" is dangerous.
//       if you really need to regenerate,please ask TA for help.
int main(int argc, const char *argv[]) {
	try {
		// TODO: please don't modify the code below the construction of ifs if you want to use visitor mode
		ANTLRInputStream input(std::cin);
		Python3Lexer lexer(&input);
		CommonTokenStream tokens(&lexer);
		tokens.fill();
		Python3Parser parser(&tokens);
		parser.removeErrorListeners();
		tree::ParseTree *tree = parser.file_input();
		if (parser.getNumberOfSyntaxErrors() > 0) {
			std::cerr << "Failed to parse input" << std::endl;
			return 1;
		}
		if (!tree) {
			std::cerr << "Parse tree is null" << std::endl;
			return 1;
		}
		EvalVisitor visitor;
		visitor.visit(tree);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
