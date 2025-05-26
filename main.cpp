#include <iostream>
#include <stdexcept>
#include <memory>
#include "ast.h"
#include <unordered_map>
#include <fstream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
using namespace llvm;



// Forward declarations from Bison parser
extern int yyparse();
extern std::unique_ptr<ASTNode> root;
extern std::unordered_map<std::string, double> symbol_table;

// Declare evaluateAST (implemented in evaluator.cpp)
extern double evaluateAST(ASTNode* node, std::unordered_map<std::string, double>& symbols);

int main() {
    std::cout << "Mathematical DSL Interpreter (type 'exit;' to quit)\n";

    // Initialize LLVM native target (if using LLVM JIT in evaluateAST)
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    if (yyparse() != 0) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }

    std::ofstream astOut("ast.txt", std::ios::app);
    if (root) {
        std::cout << "Writing AST to ast.txt\n"; // Debug line
        root->print(astOut);
        astOut << "------------------------\n";
    } else {
        std::cout << "No AST to write (root is null)\n"; // Debug line
    }


    if (root) {
        try {
            double result = evaluateAST(root.get(), symbol_table);
            std::cout << "Final result: " << result << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "Evaluation error: " << ex.what() << std::endl;
        }
    }

    

    return 0;
}
