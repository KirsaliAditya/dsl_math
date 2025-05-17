%{
#include "ast.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <map>
#include <vector>

using namespace std;
using SymbolTable = unordered_map<string, double>;
extern int yylex();
extern void yyerror(const char *s);
std::unique_ptr<ASTNode> root;
SymbolTable symbol_table;

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
%right '^'
%type <node> expression

%%

program:
    /* empty */
  | program statement
  ;

statement:
    VAR ID '=' expression ';' {
        double val = evaluateAST($4, symbol_table);
        symbol_table[$2] = val;
        cout << "Assigned: " << $2 << " = " << val << endl;
        free($2);
    }
  | expression ';' {
        auto* eqNode = dynamic_cast<EquationNode*>($1);
        if (eqNode) {
            try {
                // Call solveEquation and print results from symbol_table
                auto solutions = solveEquation($1, symbol_table);
                cout << "Equation solved. Solutions:" << endl;
                for (const auto& [var, val] : solutions)
                    cout << "  " << var << " = " << val << endl;
            } catch (const std::exception& e) {
                cerr << "Error: " << e.what() << endl;
            }
        } else {
            double val = evaluateAST($1, symbol_table);
            cout << "Result: " << val << endl;
        }
    }
  | error ';' {
        yyerror("Syntax error");
        yyerrok;
    }
  ;

expression:
    NUMBER          { $$ = new NumberNode($1); }
  | ID              { $$ = new VariableNode($1); free($1); }
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
  | expression '=' expression { $$ = new EquationNode(ASTNodePtr($1), ASTNodePtr($3)); }
  ;
%%

void yyerror(const char *s) {
    std::cerr << "Parse error: " << s << std::endl;
}
