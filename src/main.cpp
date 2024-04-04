#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "ast.h"

using namespace std;

extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAst>& ast);

void call_koopa(const char* res, ofstream& fout);
void visit_koopa(const koopa_raw_program_t& raw, ofstream& fout);
void visit_koopa(const koopa_raw_function_t& func, ofstream& fout);
void visit_koopa(const koopa_raw_basic_block_t& bb, ofstream& fout);
void visit_koopa(const koopa_raw_value_t& value, ofstream& fout);

void call_koopa(const char* res, ofstream& fout) {
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(res, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    visit_koopa(raw, fout);

    koopa_delete_raw_program_builder(builder);
}

void visit_koopa(const koopa_raw_program_t& raw, ofstream& fout) {
    fout << "  .text" << endl;
    fout << "  .globl main" << endl;

    for (size_t i = 0; i < raw.funcs.len; ++i) {
        assert(raw.funcs.kind == KOOPA_RSIK_FUNCTION);
        visit_koopa(reinterpret_cast<koopa_raw_function_t>(raw.funcs.buffer[i]), fout);
    }
}

void visit_koopa(const koopa_raw_function_t& func, ofstream& fout) {
    fout << "main:" << endl;

    for (size_t i = 0; i < func->bbs.len; ++i) {
        assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
        visit_koopa(reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]), fout);
    }
}

void visit_koopa(const koopa_raw_basic_block_t& bb, ofstream& fout) {
    for (size_t i = 0; i < bb->insts.len; ++i) {
        visit_koopa(reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]), fout);
    }
}

void visit_koopa(const koopa_raw_value_t& value, ofstream& fout) {
    assert(value->kind.tag == KOOPA_RVT_RETURN);
    koopa_raw_value_t ret_value = value->kind.data.ret.value;
    assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
    int32_t int_val = ret_value->kind.data.integer.value;
    fout << "  li a0, " << int_val << endl;
    fout << "  ret" << endl;
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
    string tmp = out.str();
    const char* res = tmp.c_str();

    ofstream fout(output);
    call_koopa(res, fout);
    return 0;
}