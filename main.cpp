#include <iostream>

extern int yyparse();

int main() {
    std::cout << "Mathematical DSL Interpreter (type 'exit;' to quit)\n";
    yyparse();
    return 0;
}