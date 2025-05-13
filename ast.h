#ifndef AST_H
#define AST_H

#include <cmath>
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

using SymbolTable = std::unordered_map<std::string, double>;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual double evaluate(SymbolTable& symbols) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class NumberNode : public ASTNode {
    double value;
public:
    NumberNode(double v) : value(v) {}
    double evaluate(SymbolTable&) const override { return value; }
};

class VariableNode : public ASTNode {
    std::string name;
public:
    VariableNode(std::string n) : name(std::move(n)) {}
    double evaluate(SymbolTable& symbols) const override {
        if (symbols.find(name) != symbols.end())
            return symbols[name];
        throw std::runtime_error("Undefined variable: " + name);
    }
};

class BinaryOpNode : public ASTNode {
    char op;
    ASTNodePtr left, right;
public:
    BinaryOpNode(char o, ASTNodePtr l, ASTNodePtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    double evaluate(SymbolTable& symbols) const override {
        double a = left->evaluate(symbols);
        double b = right->evaluate(symbols);
        switch (op) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': if (b == 0) throw std::runtime_error("Division by zero"); return a / b;
            case '^': return std::pow(a, b);
            default: throw std::runtime_error("Unknown operator");
        }
    }
};

class FunctionNode : public ASTNode {
    std::string func;
    ASTNodePtr arg;
public:
    FunctionNode(std::string f, ASTNodePtr a)
        : func(std::move(f)), arg(std::move(a)) {}
    double evaluate(SymbolTable& symbols) const override {
        double x = arg->evaluate(symbols);
        if (func == "sin") return std::sin(x);
        if (func == "cos") return std::cos(x);
        if (func == "log") {
            if (x <= 0) throw std::runtime_error("Logarithm of non-positive number");
            return std::log(x);
        }
        if (func == "sqrt") {
            if (x < 0) throw std::runtime_error("Square root of negative number");
            return std::sqrt(x);
        }
        throw std::runtime_error("Unknown function: " + func);
    }
};

class AssignmentNode : public ASTNode {
    std::string name;
    ASTNodePtr expr;
public:
    AssignmentNode(std::string n, ASTNodePtr e)
        : name(std::move(n)), expr(std::move(e)) {}
    double evaluate(SymbolTable& symbols) const override {
        double val = expr->evaluate(symbols);
        symbols[name] = val;
        return val;
    }
};

#endif
