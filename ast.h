#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <cmath>
#include <functional>

// Base AST node (abstract)
struct ASTNode {
    virtual ~ASTNode() = default;

    // Evaluate numeric value given variable bindings
    virtual double evaluate(const std::unordered_map<std::string, double>& vars) const = 0;

    // Collect variables used in this node
    virtual void collectVariables(std::vector<std::string>& vars) const = 0;
    
    // Get derivative with respect to a variable
    virtual std::unique_ptr<ASTNode> derivative(const std::string& var) const = 0;

    // Clone the node
    virtual std::unique_ptr<ASTNode> clone() const = 0;

    virtual void print(std::ostream& out, int indent = 0) const = 0;
};

// Number literal
struct NumberNode : ASTNode {
    double value;
    NumberNode(double v) : value(v) {}
    double evaluate(const std::unordered_map<std::string, double>&) const override { return value; }
    void collectVariables(std::vector<std::string>&) const override {}
    void print(std::ostream& out, int indent = 0) const override;
    std::unique_ptr<ASTNode> derivative(const std::string& var) const override { return std::make_unique<NumberNode>(0.0); }
    std::unique_ptr<ASTNode> clone() const override { return std::make_unique<NumberNode>(value); }
};

// Variable
struct VariableNode : ASTNode {
    std::string name;
    VariableNode(const std::string& n) : name(n) {}

    const std::string& getName() const { return name; }

    double evaluate(const std::unordered_map<std::string, double>& vars) const override;
    void collectVariables(std::vector<std::string>& vars) const override;
    void print(std::ostream& out, int indent = 0) const override;
    std::unique_ptr<ASTNode> derivative(const std::string& var) const override;
    std::unique_ptr<ASTNode> clone() const override { return std::make_unique<VariableNode>(name); }
};

// Binary operations
struct BinaryOpNode : ASTNode {
    char op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOpNode(char o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    double evaluate(const std::unordered_map<std::string, double>& vars) const override;
    void collectVariables(std::vector<std::string>& vars) const override;
    void print(std::ostream& out, int indent = 0) const override;
    std::unique_ptr<ASTNode> derivative(const std::string& var) const override;
    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<BinaryOpNode>(op, left->clone(), right->clone());
    }
};

// Functions like sin, cos, log, sqrt
struct FunctionNode : ASTNode {
    std::string funcName;
    std::unique_ptr<ASTNode> arg;
    FunctionNode(const std::string& f, std::unique_ptr<ASTNode> a);
    double evaluate(const std::unordered_map<std::string, double>& vars) const override;
    void collectVariables(std::vector<std::string>& vars) const override;
    void print(std::ostream& out, int indent = 0) const override;
    std::unique_ptr<ASTNode> derivative(const std::string& var) const override;
    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<FunctionNode>(funcName, arg->clone());
    }
};

// Equation node: lhs = rhs
struct EquationNode : ASTNode {
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
    EquationNode(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    double evaluate(const std::unordered_map<std::string, double>&) const override { return 0; }
    void collectVariables(std::vector<std::string>& vars) const override;
    void print(std::ostream& out, int indent = 0) const override;
    std::unique_ptr<ASTNode> derivative(const std::string& var) const override;
    std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<EquationNode>(lhs->clone(), rhs->clone());
    }
};

// Numerical solver for non-linear equations
class NumericalSolver {
public:
    static constexpr double EPSILON = 1e-10;
    static constexpr int MAX_ITERATIONS = 100;

    // Solve non-linear equation using Newton-Raphson method
    static double solveNewtonRaphson(
        const std::function<double(double)>& f,
        const std::function<double(double)>& df,
        double initial_guess,
        double tolerance = EPSILON
    );

    // Solve equation using bisection method
    static double solveBisection(
        const std::function<double(double)>& f,
        double a,
        double b,
        double tolerance = EPSILON
    );

    // Find all roots in an interval
    static std::vector<double> findAllRoots(
        const std::function<double(double)>& f,
        double start,
        double end,
        double step = 0.1
    );
};

// Solve linear equations given an EquationNode
std::unordered_map<std::string, double> solveLinearEquation(const EquationNode* equation);

// Solve any equation
std::unordered_map<std::string, double> solveEquation(ASTNode* node, std::unordered_map<std::string, double>& vars);

// Evaluate any ASTNode with given variable values
double evaluateAST(ASTNode* node, std::unordered_map<std::string, double>& vars);

#endif // AST_H
