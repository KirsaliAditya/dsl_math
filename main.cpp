#include <iostream>
#include <stdexcept>
#include <memory>
#include <fstream>
#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

// Forward declarations from Bison
extern int yyparse();
extern std::unique_ptr<ASTNode> root;
extern std::unordered_map<std::string, double> symbol_table;

// Generate LLVM IR for the AST and execute it via JIT, returning the result.
double evaluateAST(ASTNode* node, std::unordered_map<std::string, double>& symbols) {
    LLVMContext Context;
    auto ModulePtr = std::make_unique<Module>("expr_module", Context);
    IRBuilder<> Builder(Context);

    FunctionType *FT = FunctionType::get(Type::getDoubleTy(Context), false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, "expr", ModulePtr.get());
    BasicBlock *BB = BasicBlock::Create(Context, "entry", F);
    Builder.SetInsertPoint(BB);

    std::function<Value*(ASTNode*)> emit = [&](ASTNode* nd) -> Value* {
        if (auto *num = dynamic_cast<NumberNode*>(nd)) {
            return ConstantFP::get(Context, APFloat(num->evaluate(symbols)));
        }
        if (auto *var = dynamic_cast<VariableNode*>(nd)) {
            double v = var->evaluate(symbols);
            return ConstantFP::get(Context, APFloat(v));
        }
        if (auto *bin = dynamic_cast<BinaryOpNode*>(nd)) {
            if (bin->op == '^') {
                double aval = bin->left->evaluate(symbols);
                double bval = bin->right->evaluate(symbols);
                double res = std::pow(aval, bval);
                return ConstantFP::get(Context, APFloat(res));
            }
            Value *L = emit(bin->left.get());
            Value *R = emit(bin->right.get());
            switch (bin->op) {
                case '+': return Builder.CreateFAdd(L, R, "addtmp");
                case '-': return Builder.CreateFSub(L, R, "subtmp");
                case '*': return Builder.CreateFMul(L, R, "multmp");
                case '/': return Builder.CreateFDiv(L, R, "divtmp");
                default: throw std::runtime_error("Unknown binary operator");
            }
        }
        if (auto *func = dynamic_cast<FunctionNode*>(nd)) {
            double res = func->evaluate(symbols);
            return ConstantFP::get(Context, APFloat(res));
        }
        throw std::runtime_error("Unknown AST node in codegen");
    };

    Value *RetVal = emit(node);
    Builder.CreateRet(RetVal);
    std::error_code EC;
    raw_fd_ostream out("ir.ll", EC, sys::fs::OpenFlags::OF_None); // Use the correct enum
    F->getParent()->print(out, nullptr);
    verifyFunction(*F);

    InitializeNativeTarget(); 
    InitializeNativeTargetAsmPrinter(); 
    InitializeNativeTargetAsmParser();

    std::string ErrStr;
    EngineBuilder EB(std::move(ModulePtr));
    EB.setErrorStr(&ErrStr).setEngineKind(EngineKind::JIT).setMCJITMemoryManager(
        std::make_unique<SectionMemoryManager>());
    ExecutionEngine *EE = EB.create();
    if (!EE) {
        std::cerr << "Failed to create ExecutionEngine: " << ErrStr << "\n";
        throw std::runtime_error("JIT initialization failed");
    }
    EE->finalizeObject();
    auto FuncAddr = EE->getFunctionAddress("expr");
    if (!FuncAddr) {
        delete EE;
        throw std::runtime_error("Function not found in JIT");
    }
    double (*FP)() = (double (*)())FuncAddr;
    double result = FP();
    delete EE;
    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "Mathematical DSL Interpreter (type 'exit;' to quit)\n";
    InitializeNativeTarget(); 
    InitializeNativeTargetAsmPrinter(); 
    InitializeNativeTargetAsmParser();
    std::ofstream("ast.txt", std::ios::trunc).close();
    if (argc == 2) {
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        std::cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }
    extern FILE* yyin;
    yyin = file;
}
    if (yyparse() != 0) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }
    return 0;
}
