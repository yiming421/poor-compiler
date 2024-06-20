#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <cstring>
#include "koopa.h"
#include "visit.h"
#include "ast.h"

using namespace std;

extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAst>& ast);

void call_koopa(const char* res, ofstream& fout);

void call_koopa(const char* res, ofstream& fout) { // generate riscv code
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(res, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    visit_koopa(raw, fout);

    koopa_delete_raw_program_builder(builder);
}

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

    stringstream out;
    ast->dump(out);
    if (strcmp("-koopa", mode) == 0) {
        ofstream fout(output);
        fout << out.str() << endl;
    } else {
        std::stringbuf* buf = out.rdbuf();
        const string& tmp = buf->str();
        const char* res = tmp.c_str();
        ofstream fout(output);
        call_koopa(res, fout);
    }
    return 0;
}