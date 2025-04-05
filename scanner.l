%{
#include "parser.tab.h"
#include <cstring>
#include <iostream>

using namespace std;

%}

%%

"exit"          { exit(0); }
"var"           { return VAR; }
"sin"           { return SIN; }
"cos"           { return COS; }
"log"           { return LOG; }
"sqrt"          { return SQRT; }

[0-9]+(\.[0-9]+)?  { yylval.fval = atof(yytext); return NUMBER; }

[a-zA-Z_][a-zA-Z0-9_]*   { yylval.sval = strdup(yytext); return ID; }

"="             { return '='; }
";"             { return ';'; }
"("             { return '('; }
")"             { return ')'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"^"             { return '^'; }

[ \t\r\n]+      { /* skip whitespace */ }

.               { return yytext[0]; }

%%

int yywrap() {
    return 1;
}