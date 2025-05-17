#include "ast.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>

// VariableNode

double VariableNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    auto it = vars.find(name);
    if (it == vars.end())
        throw std::runtime_error("Undefined variable: " + name);
    return it->second;
}

void VariableNode::collectVariables(std::vector<std::string>& vars) const {
    vars.push_back(name);
}

// BinaryOpNode

BinaryOpNode::BinaryOpNode(char o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

double BinaryOpNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    double lval = left->evaluate(vars);
    double rval = right->evaluate(vars);
    switch (op) {
        case '+': return lval + rval;
        case '-': return lval - rval;
        case '*': return lval * rval;
        case '/': 
            if (rval == 0) throw std::runtime_error("Division by zero");
            return lval / rval;
        case '^': return std::pow(lval, rval);
        default: throw std::runtime_error("Unknown binary operator");
    }
}

void BinaryOpNode::collectVariables(std::vector<std::string>& vars) const {
    left->collectVariables(vars);
    right->collectVariables(vars);
}

// FunctionNode

FunctionNode::FunctionNode(const std::string& f, std::unique_ptr<ASTNode> a)
    : funcName(f), arg(std::move(a)) {}

double FunctionNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    double val = arg->evaluate(vars);
    if (funcName == "sin") return std::sin(val);
    if (funcName == "cos") return std::cos(val);
    if (funcName == "log") {
        if (val <= 0) throw std::runtime_error("Log domain error");
        return std::log(val);
    }
    if (funcName == "sqrt") {
        if (val < 0) throw std::runtime_error("Sqrt domain error");
        return std::sqrt(val);
    }
    throw std::runtime_error("Unknown function: " + funcName);
}

void FunctionNode::collectVariables(std::vector<std::string>& vars) const {
    arg->collectVariables(vars);
}

// EquationNode

EquationNode::EquationNode(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : lhs(std::move(l)), rhs(std::move(r)) {}

void EquationNode::collectVariables(std::vector<std::string>& vars) const {
    lhs->collectVariables(vars);
    rhs->collectVariables(vars);
}

// --- Linear equation solving helper ---

using TermMap = std::map<std::string, double>;

struct LinearExpr {
    TermMap terms; // variable -> coefficient
    double constant = 0;
};

// Recursively extract linear terms from an AST node
LinearExpr extractLinearExpr(const ASTNode* node) {
    if (auto num = dynamic_cast<const NumberNode*>(node)) {
        return LinearExpr{{}, num->value};
    }
    if (auto var = dynamic_cast<const VariableNode*>(node)) {
        return LinearExpr{{ {var->getName(), 1.0} }, 0.0};
    }
    if (auto bin = dynamic_cast<const BinaryOpNode*>(node)) {
        LinearExpr left = extractLinearExpr(bin->left.get());
        LinearExpr right = extractLinearExpr(bin->right.get());
        switch (bin->op) {
            case '+':
                for (auto& [k,v] : right.terms) left.terms[k] += v;
                left.constant += right.constant;
                return left;
            case '-':
                for (auto& [k,v] : right.terms) left.terms[k] -= v;
                left.constant -= right.constant;
                return left;
            case '*':
                if (left.terms.empty()) {
                    for(auto& [k,v] : right.terms) right.terms[k] *= left.constant;
                    right.constant *= left.constant;
                    return right;
                }
                if (right.terms.empty()) {
                    for(auto& [k,v] : left.terms) left.terms[k] *= right.constant;
                    left.constant *= right.constant;
                    return left;
                }
                throw std::runtime_error("Non-linear multiplication detected");
            case '/':
                if (!right.terms.empty())
                    throw std::runtime_error("Non-linear division detected");
                if (right.constant == 0)
                    throw std::runtime_error("Division by zero");
                for(auto& [k,v] : left.terms) left.terms[k] /= right.constant;
                left.constant /= right.constant;
                return left;
            default:
                throw std::runtime_error("Unsupported operator in linear solver");
        }
    }
    if (dynamic_cast<const FunctionNode*>(node)) {
        throw std::runtime_error("Non-linear function in linear solver");
    }
    throw std::runtime_error("Unsupported AST node in linear solver");
}

// Solve a linear equation: lhs = rhs
std::unordered_map<std::string, double> solveLinearEquation(const EquationNode* equation) {
    LinearExpr lhsExpr = extractLinearExpr(equation->lhs.get());
    LinearExpr rhsExpr = extractLinearExpr(equation->rhs.get());

    // lhs - rhs = 0
    for (auto& [k,v] : rhsExpr.terms) lhsExpr.terms[k] -= v;
    lhsExpr.constant -= rhsExpr.constant;

    if (lhsExpr.terms.empty())
        throw std::runtime_error("No variables to solve for");

    if (lhsExpr.terms.size() == 1) {
        auto [var, coeff] = *lhsExpr.terms.begin();
        if (coeff == 0)
            throw std::runtime_error("Coefficient zero, no solution");
        double val = -lhsExpr.constant / coeff;
        return { {var, val} };
    }

    throw std::runtime_error("Multiple variables not supported yet");
}

// --- Free functions ---

double evaluateAST(ASTNode* node, std::unordered_map<std::string, double>& vars) {
    if (!node) throw std::runtime_error("Null ASTNode");
    return node->evaluate(vars);
}

std::unordered_map<std::string, double> solveEquation(ASTNode* node, std::unordered_map<std::string, double>& vars) {
    auto eq = dynamic_cast<EquationNode*>(node);
    if (!eq) throw std::runtime_error("Node is not an equation");
    
    try {
        // First try linear solver
        return solveLinearEquation(eq);
    } catch (const std::runtime_error&) {
        // If linear solver fails, use numerical methods
        std::vector<std::string> variables;
        eq->collectVariables(variables);
        
        if (variables.size() != 1) {
            throw std::runtime_error("Can only solve single-variable equations numerically");
        }
        
        const std::string& var = variables[0];
        
        // Create function for f(x) = 0
        auto f = [eq, &vars, &var](double x) {
            vars[var] = x;
            return eq->lhs->evaluate(vars) - eq->rhs->evaluate(vars);
        };
        
        // Create function for f'(x)
        auto df = [eq, &vars, &var](double x) {
            vars[var] = x;
            auto derivative = eq->derivative(var);
            return derivative->evaluate(vars);
        };
        
        // Try to find all roots
        std::vector<double> roots = NumericalSolver::findAllRoots(f, -10.0, 10.0);
        
        if (roots.empty()) {
            throw std::runtime_error("No roots found in the search interval");
        }
        
        // Return the first root found
        return {{var, roots[0]}};
    }
}

// Derivative implementations
std::unique_ptr<ASTNode> VariableNode::derivative(const std::string& var) const {
    if (name == var) {
        return std::make_unique<NumberNode>(1.0);
    }
    return std::make_unique<NumberNode>(0.0);
}

std::unique_ptr<ASTNode> BinaryOpNode::derivative(const std::string& var) const {
    auto dleft = left->derivative(var);
    auto dright = right->derivative(var);
    
    switch (op) {
        case '+':
            return std::make_unique<BinaryOpNode>('+', std::move(dleft), std::move(dright));
        case '-':
            return std::make_unique<BinaryOpNode>('-', std::move(dleft), std::move(dright));
        case '*': {
            // Product rule: (f*g)' = f'*g + f*g'
            auto term1 = std::make_unique<BinaryOpNode>('*', std::move(dleft), right->clone());
            auto term2 = std::make_unique<BinaryOpNode>('*', left->clone(), std::move(dright));
            return std::make_unique<BinaryOpNode>('+', std::move(term1), std::move(term2));
        }
        case '/': {
            // Quotient rule: (f/g)' = (f'*g - f*g')/g^2
            auto numerator1 = std::make_unique<BinaryOpNode>('*', std::move(dleft), right->clone());
            auto numerator2 = std::make_unique<BinaryOpNode>('*', left->clone(), std::move(dright));
            auto numerator = std::make_unique<BinaryOpNode>('-', std::move(numerator1), std::move(numerator2));
            auto denominator = std::make_unique<BinaryOpNode>('^', right->clone(), std::make_unique<NumberNode>(2.0));
            return std::make_unique<BinaryOpNode>('/', std::move(numerator), std::move(denominator));
        }
        case '^': {
            // Power rule: (x^n)' = n*x^(n-1)
            if (auto* num = dynamic_cast<NumberNode*>(right.get())) {
                auto newPower = std::make_unique<NumberNode>(num->value - 1);
                auto powerTerm = std::make_unique<BinaryOpNode>('^', left->clone(), std::move(newPower));
                return std::make_unique<BinaryOpNode>('*', 
                    std::make_unique<NumberNode>(num->value),
                    std::move(powerTerm));
            }
            throw std::runtime_error("Power rule only implemented for constant exponents");
        }
        default:
            throw std::runtime_error("Unknown operator in derivative");
    }
}

std::unique_ptr<ASTNode> FunctionNode::derivative(const std::string& var) const {
    auto darg = arg->derivative(var);
    
    if (funcName == "sin") {
        auto cosTerm = std::make_unique<FunctionNode>("cos", arg->clone());
        return std::make_unique<BinaryOpNode>('*', std::move(darg), std::move(cosTerm));
    }
    if (funcName == "cos") {
        auto sinTerm = std::make_unique<FunctionNode>("sin", arg->clone());
        auto negSin = std::make_unique<BinaryOpNode>('*', std::make_unique<NumberNode>(-1.0), std::move(sinTerm));
        return std::make_unique<BinaryOpNode>('*', std::move(darg), std::move(negSin));
    }
    if (funcName == "log") {
        return std::make_unique<BinaryOpNode>('/', std::move(darg), arg->clone());
    }
    if (funcName == "sqrt") {
        auto sqrtTerm = std::make_unique<FunctionNode>("sqrt", arg->clone());
        return std::make_unique<BinaryOpNode>('/', 
            std::move(darg),
            std::make_unique<BinaryOpNode>('*', 
                std::make_unique<NumberNode>(2.0),
                std::move(sqrtTerm)));
    }
    throw std::runtime_error("Unknown function in derivative");
}

std::unique_ptr<ASTNode> EquationNode::derivative(const std::string& var) const {
    auto dlhs = lhs->derivative(var);
    auto drhs = rhs->derivative(var);
    return std::make_unique<EquationNode>(std::move(dlhs), std::move(drhs));
}

// Numerical solver implementations
double NumericalSolver::solveNewtonRaphson(
    const std::function<double(double)>& f,
    const std::function<double(double)>& df,
    double initial_guess,
    double tolerance) {
    
    double x = initial_guess;
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        double fx = f(x);
        if (std::abs(fx) < tolerance) {
            return x;
        }
        double dfx = df(x);
        if (std::abs(dfx) < tolerance) {
            throw std::runtime_error("Derivative too close to zero");
        }
        double dx = fx / dfx;
        x -= dx;
        if (std::abs(dx) < tolerance) {
            return x;
        }
    }
    throw std::runtime_error("Newton-Raphson method did not converge");
}

double NumericalSolver::solveBisection(
    const std::function<double(double)>& f,
    double a,
    double b,
    double tolerance) {
    
    double fa = f(a);
    double fb = f(b);
    
    if (fa * fb > 0) {
        throw std::runtime_error("Function values at endpoints must have opposite signs");
    }
    
    while (b - a > tolerance) {
        double c = (a + b) / 2;
        double fc = f(c);
        
        if (std::abs(fc) < tolerance) {
            return c;
        }
        
        if (fa * fc < 0) {
            b = c;
            fb = fc;
        } else {
            a = c;
            fa = fc;
        }
    }
    
    return (a + b) / 2;
}

std::vector<double> NumericalSolver::findAllRoots(
    const std::function<double(double)>& f,
    double start,
    double end,
    double step) {
    
    std::vector<double> roots;
    double x = start;
    double prev_fx = f(x);
    
    while (x < end) {
        x += step;
        double fx = f(x);
        
        if (prev_fx * fx <= 0) {
            try {
                double root = solveBisection(f, x - step, x);
                if (std::find_if(roots.begin(), roots.end(),
                    [root](double r) { return std::abs(r - root) < EPSILON; }) == roots.end()) {
                    roots.push_back(root);
                }
            } catch (const std::exception&) {
                // Skip if bisection fails
            }
        }
        
        prev_fx = fx;
    }
    
    return roots;
}
