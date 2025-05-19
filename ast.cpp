#include "ast.h"
#include <stdexcept>
#include <set>

// VariableNode implementation
double VariableNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    auto it = vars.find(name);
    if (it == vars.end()) throw std::runtime_error("Variable not defined: " + name);
    return it->second;
}

void VariableNode::collectVariables(std::vector<std::string>& vars) const {
    vars.push_back(name);
}

std::unique_ptr<ASTNode> VariableNode::derivative(const std::string& var) const {
    return std::make_unique<NumberNode>(name == var ? 1.0 : 0.0);
}

// BinaryOpNode implementation
BinaryOpNode::BinaryOpNode(char o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

double BinaryOpNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    double l = left->evaluate(vars);
    double r = right->evaluate(vars);
    switch (op) {
        case '+': return l + r;
        case '-': return l - r;
        case '*': return l * r;
        case '/': return r != 0 ? l / r : throw std::runtime_error("Division by zero");
        case '^': return std::pow(l, r);
        default: throw std::runtime_error("Unknown binary operator");
    }
}

void BinaryOpNode::collectVariables(std::vector<std::string>& vars) const {
    left->collectVariables(vars);
    right->collectVariables(vars);
}

std::unique_ptr<ASTNode> BinaryOpNode::derivative(const std::string& var) const {
    if (op == '+') return std::make_unique<BinaryOpNode>('+', left->derivative(var), right->derivative(var));
    if (op == '-') return std::make_unique<BinaryOpNode>('-', left->derivative(var), right->derivative(var));
    if (op == '*') {
        return std::make_unique<BinaryOpNode>('+',
            std::make_unique<BinaryOpNode>('*', left->derivative(var), right->clone()),
            std::make_unique<BinaryOpNode>('*', left->clone(), right->derivative(var))
        );
    }
    if (op == '/') {
        return std::make_unique<BinaryOpNode>('/',
            std::make_unique<BinaryOpNode>('-',
                std::make_unique<BinaryOpNode>('*', left->derivative(var), right->clone()),
                std::make_unique<BinaryOpNode>('*', left->clone(), right->derivative(var))
            ),
            std::make_unique<BinaryOpNode>('*', right->clone(), right->clone())
        );
    }
    if (op == '^') {
        return std::make_unique<BinaryOpNode>('*',
            std::make_unique<BinaryOpNode>('*',
                std::make_unique<BinaryOpNode>('^', left->clone(), right->clone()),
                right->clone()
            ),
            std::make_unique<BinaryOpNode>('/',
                left->derivative(var),
                left->clone()
            )
        );
    }
    throw std::runtime_error("Unsupported operation for derivative");
}

// FunctionNode implementation
FunctionNode::FunctionNode(const std::string& f, std::unique_ptr<ASTNode> a)
    : funcName(f), arg(std::move(a)) {}

double FunctionNode::evaluate(const std::unordered_map<std::string, double>& vars) const {
    double val = arg->evaluate(vars);
    if (funcName == "sin") return std::sin(val);
    if (funcName == "cos") return std::cos(val);
    if (funcName == "log") return std::log(val);
    if (funcName == "sqrt") return std::sqrt(val);
    throw std::runtime_error("Unknown function: " + funcName);
}

void FunctionNode::collectVariables(std::vector<std::string>& vars) const {
    arg->collectVariables(vars);
}

std::unique_ptr<ASTNode> FunctionNode::derivative(const std::string& var) const {
    if (funcName == "sin") {
        return std::make_unique<BinaryOpNode>('*',
            std::make_unique<FunctionNode>("cos", arg->clone()),
            arg->derivative(var)
        );
    }
    if (funcName == "cos") {
        return std::make_unique<BinaryOpNode>('*',
            std::make_unique<NumberNode>(-1.0),
            std::make_unique<BinaryOpNode>('*',
                std::make_unique<FunctionNode>("sin", arg->clone()),
                arg->derivative(var)
            )
        );
    }
    if (funcName == "log") {
        return std::make_unique<BinaryOpNode>('/',
            arg->derivative(var),
            arg->clone()
        );
    }
    if (funcName == "sqrt") {
        return std::make_unique<BinaryOpNode>('/',
            arg->derivative(var),
            std::make_unique<BinaryOpNode>('*',
                std::make_unique<NumberNode>(2.0),
                std::make_unique<FunctionNode>("sqrt", arg->clone())
            )
        );
    }
    throw std::runtime_error("Derivative not implemented for function: " + funcName);
}

// EquationNode implementation
EquationNode::EquationNode(std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
    : lhs(std::move(l)), rhs(std::move(r)) {}

void EquationNode::collectVariables(std::vector<std::string>& vars) const {
    lhs->collectVariables(vars);
    rhs->collectVariables(vars);
}

std::unique_ptr<ASTNode> EquationNode::derivative(const std::string& var) const {
    return std::make_unique<EquationNode>(lhs->derivative(var), rhs->derivative(var));
}

// Solver implementations
double NumericalSolver::solveNewtonRaphson(
    const std::function<double(double)>& f,
    const std::function<double(double)>& df,
    double guess,
    double tol
) {
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        double fval = f(guess);
        double dfval = df(guess);
        if (std::abs(dfval) < EPSILON) throw std::runtime_error("Derivative near zero");
        double next = guess - fval / dfval;
        if (std::abs(next - guess) < tol) return next;
        guess = next;
    }
    throw std::runtime_error("Newton-Raphson did not converge");
}

double NumericalSolver::solveBisection(
    const std::function<double(double)>& f,
    double a,
    double b,
    double tol
) {
    double fa = f(a), fb = f(b);
    if (fa * fb >= 0) throw std::runtime_error("Bisection method requires opposite signs");

    while ((b - a) / 2.0 > tol) {
        double mid = (a + b) / 2.0;
        double fmid = f(mid);
        if (fmid == 0) return mid;
        else if (fa * fmid < 0) {
            b = mid;
            fb = fmid;
        } else {
            a = mid;
            fa = fmid;
        }
    }
    return (a + b) / 2.0;
}

std::vector<double> NumericalSolver::findAllRoots(
    const std::function<double(double)>& f,
    double start,
    double end,
    double step
) {
    std::vector<double> roots;
    for (double x = start; x < end; x += step) {
        double x1 = x, x2 = x + step;
        try {
            double y1 = f(x1), y2 = f(x2);
            if (y1 * y2 <= 0) {
                double root = solveBisection(f, x1, x2);
                roots.push_back(root);
            }
        } catch (...) {
            continue;
        }
    }
    return roots;
}

// Evaluate AST
double evaluateAST(ASTNode* node, std::unordered_map<std::string, double>& vars) {
    return node->evaluate(vars);
}

// Solve linear equations
std::unordered_map<std::string, double> solveLinearEquation(const EquationNode* eq) {
    std::vector<std::string> vars;
    eq->collectVariables(vars);
    std::set<std::string> unique(vars.begin(), vars.end());
    if (unique.size() != 1) throw std::runtime_error("Only one variable supported for linear solver");

    std::string var = *unique.begin();
    auto derivativeNode = std::make_unique<BinaryOpNode>('-',
        eq->lhs->clone(), eq->rhs->clone()
    );

    auto dNode = derivativeNode->derivative(var);
    std::function<double(double)> f = [&](double x) {
        std::unordered_map<std::string, double> vmap = {{var, x}};
        return derivativeNode->evaluate(vmap);
    };
    std::function<double(double)> df = [&](double x) {
        std::unordered_map<std::string, double> vmap = {{var, x}};
        return dNode->evaluate(vmap);
    };

    double root = NumericalSolver::solveNewtonRaphson(f, df, 1.0);
    return {{var, root}};
}

// Helper function to check if an expression is a power expression
bool isPowerExpression(const ASTNode* node, const std::string& var) {
    if (auto* binOp = dynamic_cast<const BinaryOpNode*>(node)) {
        if (binOp->op == '^') {
            // Check if right side is a number
            if (auto* rightNum = dynamic_cast<const NumberNode*>(binOp->right.get())) {
                // Check if left side is the variable we're solving for
                if (auto* leftVar = dynamic_cast<const VariableNode*>(binOp->left.get())) {
                    return leftVar->name == var;
                }
            }
        }
    }
    return false;
}

// Helper function to get the power from a power expression
double getPowerFromExpression(const ASTNode* node) {
    if (auto* binOp = dynamic_cast<const BinaryOpNode*>(node)) {
        if (binOp->op == '^') {
            if (auto* rightNum = dynamic_cast<const NumberNode*>(binOp->right.get())) {
                return rightNum->value;
            }
        }
    }
    return 0.0;
}

// Helper function to get the base from a power expression
double getBaseFromExpression(const ASTNode* node) {
    if (auto* binOp = dynamic_cast<const BinaryOpNode*>(node)) {
        if (binOp->op == '^') {
            if (auto* leftNum = dynamic_cast<const NumberNode*>(binOp->left.get())) {
                return leftNum->value;
            }
        }
    }
    return 0.0;
}

// General equation solver
std::unordered_map<std::string, double> solveEquation(ASTNode* node, std::unordered_map<std::string, double>& vars) {
    EquationNode* eq = dynamic_cast<EquationNode*>(node);
    if (!eq) throw std::runtime_error("Not an equation node");

    // Collect variables
    std::vector<std::string> variables;
    eq->collectVariables(variables);
    std::set<std::string> unique_vars(variables.begin(), variables.end());
    
    if (unique_vars.size() != 1) {
        throw std::runtime_error("Can only solve single-variable equations numerically");
    }
    
    std::string var = *unique_vars.begin();

    // Check if it's a power equation (x^n = c)
    bool isPowerEq = false;
    double power = 0.0;
    double constant = 0.0;

    // Check if left side is a power expression
    if (isPowerExpression(eq->lhs.get(), var)) {
        power = getPowerFromExpression(eq->lhs.get());
        if (auto* rightNum = dynamic_cast<const NumberNode*>(eq->rhs.get())) {
            constant = rightNum->value;
            isPowerEq = true;
        }
    }
    // Check if right side is a power expression
    else if (isPowerExpression(eq->rhs.get(), var)) {
        power = getPowerFromExpression(eq->rhs.get());
        if (auto* leftNum = dynamic_cast<const NumberNode*>(eq->lhs.get())) {
            constant = leftNum->value;
            isPowerEq = true;
        }
    }

    if (isPowerEq) {
        std::unordered_map<std::string, double> solutions;
        
        // For even powers, we can have both positive and negative roots
        if (static_cast<int>(power) % 2 == 0) {
            double root = std::pow(constant, 1.0/power);
            solutions[var] = root;
            solutions[var + "_neg"] = -root;
        }
        // For odd powers, we only have one real root
        else {
            double root = std::pow(constant, 1.0/power);
            solutions[var] = root;
        }
        return solutions;
    }

    // If not a power equation, try linear solver first
    try {
        return solveLinearEquation(eq);
    } catch (const std::exception&) {
        // If linear solver fails, use numerical methods
        auto derivativeNode = std::make_unique<BinaryOpNode>('-',
            eq->lhs->clone(), eq->rhs->clone()
        );

        std::function<double(double)> f = [&](double x) {
            std::unordered_map<std::string, double> vmap = {{var, x}};
            return derivativeNode->evaluate(vmap);
        };

        std::vector<double> roots;
        try {
            // Try Newton-Raphson with multiple starting points
            for (double x0 : {-10.0, -5.0, -1.0, 0.0, 1.0, 5.0, 10.0}) {
                try {
                    auto dNode = derivativeNode->derivative(var);
                    std::function<double(double)> df = [&](double x) {
                        std::unordered_map<std::string, double> vmap = {{var, x}};
                        return dNode->evaluate(vmap);
                    };
                    double root = NumericalSolver::solveNewtonRaphson(f, df, x0);
                    roots.push_back(root);
                } catch (...) {
                    continue;
                }
            }
        } catch (...) {
            // If Newton-Raphson fails, try bisection
            roots = NumericalSolver::findAllRoots(f, -10.0, 10.0, 0.1);
        }

        std::unordered_map<std::string, double> solutions;
        for (size_t i = 0; i < roots.size(); ++i) {
            std::string varName = i == 0 ? var : var + "_" + std::to_string(i);
            solutions[varName] = roots[i];
        }
        return solutions;
    }
}
