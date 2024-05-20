#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include "koopa.h"
#include "visit.h"

using namespace std;
int tracktmp = 0;

class stack {
public:
    stack() {}

    int precompute(const koopa_raw_function_t& func) {
        for (size_t i = 0; i < func->bbs.len; ++i) {
            assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
            koopa_raw_basic_block_t bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
            for (size_t j = 0; j < bb->insts.len; ++j) {
                koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
                if (value->kind.tag == KOOPA_RVT_BINARY) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_ALLOC) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_LOAD) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_STORE && value->kind.data.store.value->kind.tag != KOOPA_RVT_INTEGER) {
                    var2bias[value] = bias;
                    bias += 4;
                }
            }
        }
        bias = ceil(static_cast<double>(bias) / 16) * 16;
        return bias;
    }

    int getbias(const koopa_raw_value_t& value) {
        return var2bias[value];
    }

    int clear() {
        int tmp = bias;
        bias = 0;
        var2bias.clear();
        return tmp;
    }
    int getnum() {
        return bias;
    }

private:
    int bias = 0;
    unordered_map<koopa_raw_value_t, int> var2bias;
}; 

stack st;

const vector<string> op_name = {
    "", "", "sgt", "slt", "", "", "add", "sub", "mul", "div", "rem", 
    "and", "or", "xor", "sll", "srl", "sra"
};

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
    fout << "  li t0, -" << st.precompute(func) << endl;
    fout << "  addi sp, sp, t0" << endl;

    for (size_t i = 0; i < func->bbs.len; ++i) {
        assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
        visit_koopa(reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]), fout);
    }
    st.clear();
}

void visit_koopa(const koopa_raw_basic_block_t& bb, ofstream& fout) {
    if (strcmp(bb->name, "%entry")) {
        fout << string(bb->name + 1) << ":" << endl;
    }
    for (size_t i = 0; i < bb->insts.len; ++i) {
        visit_koopa(reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]), fout);
    }
}

void visit_koopa(const koopa_raw_value_t& value, ofstream& fout) {
    switch(value->kind.tag) {
        case KOOPA_RVT_RETURN:
            visit_koopa(value->kind.data.ret, fout);
            break;
        case KOOPA_RVT_INTEGER:
            visit_koopa(value->kind.data.integer, fout);
            break;
        case KOOPA_RVT_BINARY:
            visit_koopa(value->kind.data.binary, fout);
            fout << "  sw t0, " << st.getbias(value) << "(sp)" << endl;
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_LOAD:
            visit_koopa(value->kind.data.load, fout);
            fout << "  sw t0, " << st.getbias(value) << "(sp)" << endl;
            break;
        case KOOPA_RVT_STORE:
            visit_koopa(value->kind.data.store, fout);
            break;
        case KOOPA_RVT_JUMP:
            visit_koopa(value->kind.data.jump, fout);
            break;
        case KOOPA_RVT_BRANCH:
            visit_koopa(value->kind.data.branch, fout);
            break;
        default:
            assert(false);
    }
}

void visit_koopa(const koopa_raw_jump_t& jump, ofstream& fout) {
    fout << "  j " << string(jump.target->name + 1) << endl;
}

void visit_koopa(const koopa_raw_branch_t& branch, ofstream& fout) {
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER) {
        fout << "  li t0, " << branch.cond->kind.data.integer.value << endl;
    } else {
        fout << "  lw t0, " << st.getbias(branch.cond) << "(sp)" << endl;
    }
    string tmplabel = "L_";
    tmplabel += to_string(tracktmp++);
    fout << "  bnez t0, " << tmplabel << endl;
    fout << "  j " << string(branch.false_bb->name + 1) << endl;
    fout << tmplabel << ":" << endl;
    fout << "  j " << string(branch.true_bb->name + 1) << endl; 
}

void visit_koopa(const koopa_raw_load_t& load, ofstream& fout) {
    fout << "  lw t0, " << st.getbias(load.src) << "(sp)" << endl;
}

void visit_koopa(const koopa_raw_store_t& store, ofstream& fout) {
    if (store.value->kind.tag == KOOPA_RVT_INTEGER) {
        fout << "  li t0, " << store.value->kind.data.integer.value << endl;
    } else {
        fout << "  lw t0, " << st.getbias(store.value) << "(sp)" << endl;
    }
    fout << "  sw t0, " << st.getbias(store.dest) << "(sp)" << endl;
}

void visit_koopa(const koopa_raw_return_t& ret, ofstream& fout) {
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        fout << "  li a0, " << ret.value->kind.data.integer.value << endl;
    } else {
        fout << "  lw a0, " << st.getbias(ret.value) << "(sp)" << endl;
    }
    fout << "  li t0, " << st.getnum() << endl;
    fout << "  addi sp, sp, t0" << endl;
    fout << "  ret" << endl;
}

int visit_koopa(const koopa_raw_integer_t& integer, ofstream& fout) {
    return integer.value;
}

void visit_koopa(const koopa_raw_binary_t& binary, ofstream& fout) {
    koopa_raw_value_t lhs = binary.lhs, rhs = binary.rhs;
    if (lhs->kind.tag == KOOPA_RVT_INTEGER) {
        fout << "  li t0, " << visit_koopa(lhs->kind.data.integer, fout) << endl;
    } else {
        fout << "  lw t0, " << st.getbias(lhs) << "(sp)" << endl;
    }
    if (rhs->kind.tag == KOOPA_RVT_INTEGER) {
        fout << "  li t1, " << visit_koopa(rhs->kind.data.integer, fout) << endl;
    } else {
        fout << "  lw t1, " << st.getbias(rhs) << "(sp)" << endl;
    }
    switch (binary.op) {
        case KOOPA_RBO_EQ:
            fout << "  xor t0, t0, t1" << endl;
            fout << "  seqz t0, t0" << endl;
            break;
        case KOOPA_RBO_NOT_EQ:
            fout << "  xor t0, t0, t1" << endl;
            fout << "  snez t0, t0" << endl;
            break;
        case KOOPA_RBO_LE:
            fout << "  sgt t0, t0, t1" << endl;
            fout << "  seqz t0, t0" << endl;
            break;
        case KOOPA_RBO_GE:
            fout << "  slt t0, t0, t1" << endl;
            fout << "  seqz t0, t0" << endl;
            break;
        default:
            fout << "  " << op_name[static_cast<int>(binary.op)] << " t0, t0, t1" << endl;
            break;
    }
}
