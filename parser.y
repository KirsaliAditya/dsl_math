%{
#include "ast.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <map>
#include <vector>
#include <fstream>

using namespace std;
using SymbolTable = unordered_map<string, double>;
extern int yylex();
extern void yyerror(const char *s);
std::unique_ptr<ASTNode> root;
SymbolTable symbol_table;
std::unordered_map<string, bool> declared_variables;

// Only declare evaluateAST and solveEquation with void return (as per ast.h)
double evaluateAST(ASTNode* node, SymbolTable& symbols);
std::unordered_map<std::string, double> solveEquation(ASTNode* eq, SymbolTable& solutions);
%}

%define parse.error verbose

%code requires {
    #include "ast.h"
    using ASTNode = ASTNode;
    using ASTNodePtr = std::unique_ptr<ASTNode>;
}

%union {
    ASTNode* node;
    double    fval;
    char*     sval;
}

%token <fval> NUMBER
%token <sval> ID
%token VAR SIN COS LOG SQRT
%left '+' '-'
%left '*' '/'
%right UMINUS
%right '^'
%type <node> expression equation

%%

program:
    /* empty */
  | program statement
  ;

statement:
    VAR ID ';' {
        declared_variables[$2] = true;
        symbol_table[$2] = 0.0;  // Initialize with 0
        cout << "Declared variable: " << $2 << endl;
        free($2);
    }
    | VAR ID '=' expression ';' {
        declared_variables[$2] = true;
        double val = evaluateAST($4, symbol_table);
        root = std::make_unique<VariableNode>($2); // simulate root for printing
        symbol_table[$2] = val;
        cout << "Declared and initialized variable: " << $2 << " = " << val << endl;

        std::ofstream out("ast.txt", std::ios::app);
        if ($4) {
            $4->print(out);
            out << "------------------------\n";
        }

        free($2);
    }
  | equation ';' {
        root.reset($1);
        try {
            std::vector<std::string> vars;
            $1->collectVariables(vars);
            for (const auto& var : vars) {
                if (!declared_variables[var]) {
                    throw std::runtime_error("Variable '" + var + "' must be declared with 'var' before use");
                }
            }

            auto solutions = solveEquation($1, symbol_table);
            if (solutions.empty()) {
                cout << "No solutions found" << endl;
            } else {
                cout << "Equation solved. Solutions:" << endl;
                for (const auto& [var, val] : solutions) {
                    cout << "  " << var << " = " << val << endl;
                    symbol_table[var] = val;
                }
            }

            std::ofstream out("ast.txt", std::ios::app);
            root->print(out);
            out << "------------------------\n";

        } catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
  | expression ';' {
        root.reset($1);
        try {
            double val = evaluateAST($1, symbol_table);
            cout << "Result: " << val << endl;

            std::ofstream out("ast.txt", std::ios::app);
            root->print(out);
            out << "------------------------\n";

        } catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
    ;

equation:
    expression '=' expression { $$ = new EquationNode(ASTNodePtr($1), ASTNodePtr($3)); }
  ;

expression:
    NUMBER          { $$ = new NumberNode($1); }
  | ID { 
        if (!declared_variables[$1]) {
            yyerror(("Variable '" + std::string($1) + "' must be declared with 'var' before use").c_str());
            YYERROR;
        }
        $$ = new VariableNode($1); 
        free($1); 
    }
  | '-' expression %prec UMINUS { $$ = new BinaryOpNode('*', std::make_unique<NumberNode>(-1.0), ASTNodePtr($2)); }
  | expression '+' expression { $$ = new BinaryOpNode('+', ASTNodePtr($1), ASTNodePtr($3)); }
  | expression '-' expression { $$ = new BinaryOpNode('-', ASTNodePtr($1), ASTNodePtr($3)); }
  | expression '*' expression { $$ = new BinaryOpNode('*', ASTNodePtr($1), ASTNodePtr($3)); }
  | expression '/' expression { $$ = new BinaryOpNode('/', ASTNodePtr($1), ASTNodePtr($3)); }
  | expression '^' expression { $$ = new BinaryOpNode('^', ASTNodePtr($1), ASTNodePtr($3)); }
  | SIN '(' expression ')' { $$ = new FunctionNode("sin", ASTNodePtr($3)); }
  | COS '(' expression ')' { $$ = new FunctionNode("cos", ASTNodePtr($3)); }
  | LOG '(' expression ')' { $$ = new FunctionNode("log", ASTNodePtr($3)); }
  | SQRT '(' expression ')' { $$ = new FunctionNode("sqrt", ASTNodePtr($3)); }
  | '(' expression ')' { $$ = $2; }
  ;

%%

void yyerror(const char *s) {
    std::cerr << "Parse error: " << s << std::endl;
}
