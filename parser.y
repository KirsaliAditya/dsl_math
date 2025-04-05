%{
#include <cstdio>
#include <cmath>
#include <map>
#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

map<string, double> symbol_table;

int yylex();
void yyerror(const char *s);
%}

%union {
    double fval;
    char* sval;
}

%token <fval> NUMBER
%token <sval> ID
%token VAR SIN COS LOG SQRT
%left '+' '-'
%left '*' '/'
%right '^'
%type <fval> expression

%%

program:
    program statement
    | /* empty */
    ;

statement:
    VAR ID '=' expression ';'  {
        symbol_table[$2] = $4;
        cout << "Assigned: " << $2 << " = " << $4 << endl;
        free($2);
    }
    | expression ';' {
        cout << "Result: " << $1 << endl;
    }
    | error ';' {
        yyerrok;
    }
    ;

expression:
    NUMBER { $$ = $1; }
    | ID    {
        if (symbol_table.find($1) != symbol_table.end()) {
            $$ = symbol_table[$1];
        } else {
            cerr << "Error: Undefined variable '" << $1 << "'" << endl;
            $$ = 0;
        }
        free($1);
    }
    | expression '+' expression { $$ = $1 + $3; }
    | expression '-' expression { $$ = $1 - $3; }
    | expression '*' expression { $$ = $1 * $3; }
    | expression '/' expression {
        if ($3 == 0) {
            cerr << "Error: Division by zero" << endl;
            $$ = 0;
        } else {
            $$ = $1 / $3;
        }
    }
    | expression '^' expression { $$ = pow($1, $3); }
    | SIN '(' expression ')'    { $$ = sin($3); }
    | COS '(' expression ')'    { $$ = cos($3); }
    | LOG '(' expression ')'    {
        if ($3 <= 0) {
            cerr << "Error: Logarithm of non-positive number" << endl;
            $$ = 0;
        } else {
            $$ = log($3);
        }
    }
    | SQRT '(' expression ')'   {
        if ($3 < 0) {
            cerr << "Error: Square root of negative number" << endl;
            $$ = 0;
        } else {
            $$ = sqrt($3);
        }
    }
    | '(' expression ')'        { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    cerr << "Syntax Error: " << s << endl;
}