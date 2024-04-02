#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.h"

using namespace std;

extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAst>& ast);

int main(int argc, char** argv) {
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    yyin = fopen(input, "r");
    assert(yyin != nullptr);

    unique_ptr<BaseAst> ast;
    int ret = yyparse(ast);
    assert(!ret);
    
    ofstream out(output);
    ast->dump(out);
    out.close();
    return 0;
}