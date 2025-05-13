%{
#include "ast.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <iostream>
#include <memory>

using namespace std;

using SymbolTable = unordered_map<string, double>;
SymbolTable symbol_table;

int yylex();
void yyerror(const char *s);

ASTNodePtr root; // root of AST
%}

%define parse.error verbose

%code requires {
    #include "ast.h"
    using ASTNode = ASTNode;
    using ASTNodePtr = std::unique_ptr<ASTNode>;
}

%union {
    ASTNode* node;
    double fval;
    char* sval;
}

%token <fval> NUMBER
%token <sval> ID
%token VAR SIN COS LOG SQRT
%left '+' '-'
%left '*' '/'
%right '^'
%type <node> expression statement

%%

program:
    program statement
    | /* empty */
    ;

statement:
    VAR ID '=' expression ';' {
        root = std::make_unique<AssignmentNode>($2, ASTNodePtr($4));
        try {
            double result = root->evaluate(symbol_table);
            cout << "Assigned: " << $2 << " = " << result << endl;
        } catch (exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
        free($2);
    }
    | expression ';' {
        root = ASTNodePtr($1);
        try {
            double result = root->evaluate(symbol_table);
            cout << "Result: " << result << endl;
        } catch (exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
    | error ';' {
        yyerrok;
    }
    ;

expression:
    NUMBER { $$ = new NumberNode($1); }
    | ID {
        $$ = new VariableNode($1);
        free($1);
    }
    | expression '+' expression { $$ = new BinaryOpNode('+', ASTNodePtr($1), ASTNodePtr($3)); }
    | expression '-' expression { $$ = new BinaryOpNode('-', ASTNodePtr($1), ASTNodePtr($3)); }
    | expression '*' expression { $$ = new BinaryOpNode('*', ASTNodePtr($1), ASTNodePtr($3)); }
    | expression '/' expression { $$ = new BinaryOpNode('/', ASTNodePtr($1), ASTNodePtr($3)); }
    | expression '^' expression { $$ = new BinaryOpNode('^', ASTNodePtr($1), ASTNodePtr($3)); }
    | SIN '(' expression ')'    { $$ = new FunctionNode("sin", ASTNodePtr($3)); }
    | COS '(' expression ')'    { $$ = new FunctionNode("cos", ASTNodePtr($3)); }
    | LOG '(' expression ')'    { $$ = new FunctionNode("log", ASTNodePtr($3)); }
    | SQRT '(' expression ')'   { $$ = new FunctionNode("sqrt", ASTNodePtr($3)); }
    | '(' expression ')'        { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    cerr << "Syntax Error: " << s << endl;
}
