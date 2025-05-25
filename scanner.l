%{
    #include "parser.tab.hpp"
    #include <cstring>
    #include <fstream>
    std::ofstream token_out("tokens.txt");
%}

%%
[ \t\r\n]+              ;  // Ignore whitespace

"exit"                  {exit(0);}
"var"                   { token_out << "VAR\n"; return VAR; }
"sin"                   { token_out << "SIN\n"; return SIN; }
"cos"                   { token_out << "COS\n"; return COS; }
"log"                   { token_out << "LOG\n"; return LOG; }
"sqrt"                  { token_out << "SQRT\n"; return SQRT; }

"="                     { token_out << "ASSIGNMENT\n"; return '='; }
"("                     { token_out << "LEFT P\n"; return '('; }
")"                     { token_out << "RIGHT P\n"; return ')'; }
"{"                     { token_out << "LBRACE\n"; return '{'; }
"}"                     { token_out << "RBRACE\n"; return '}'; }
";"                     { token_out << "SEMICOLON\n"; return ';'; }
","                     { token_out << "COMMA\n"; return ','; }

"+"                     { token_out << "PLUS\n"; return '+'; }
"-"                     { token_out << "MINUS\n"; return '-'; }
"*"                     { token_out << "MULTIPLY\n"; return '*'; }
"/"                     { token_out << "DIV\n"; return '/'; }

[0-9]+(\.[0-9]+)?       {
                          token_out << "NUMBER(" << yytext << ")\n";
                          yylval.fval = atof(yytext);
                          return NUMBER;
                        }

[a-zA-Z_][a-zA-Z0-9_]*   {
                          token_out << "ID(" << yytext << ")\n";
                          yylval.sval = strdup(yytext);
                          return ID;
                        }

.                       { token_out << "UNKNOWN(" << yytext << ")\n"; return yytext[0]; }

%%

int yywrap() {
    token_out.close();
    return 1;
}
