#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include "koopa.h"
#include "visit.h"
#include "util.h"

using namespace std;
int tracktmp = 0;
RiscvPrinter riscv_printer;
bool flag_ra = false;

class stack {
public:
    stack() {}

    int precompute(const koopa_raw_function_t& func) {
        for (size_t i = 0; i < func->params.len; ++i) {
            koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
            var2bias[value] = bias;
        }
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
                } else if (value->kind.tag == KOOPA_RVT_CALL) {
                    var2bias[value] = bias;
                    bias += 4;
                    flag_ra = true;
                    if (value->kind.data.call.args.len > 8) {
                        params_num = std::max(params_num, static_cast<int>(value->kind.data.call.args.len));
                    }
                }
            }
        }
        if (flag_ra) {
            bias += 4;
        }
        if (params_num > 8) {
            bias += 4 * (params_num - 8);
            for (auto& pair: var2bias) {
                var2bias[pair.first] += 4 * (params_num - 8);
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
        params_num = 0;
        return tmp;
    }
    int getnum() {
        return bias;
    }

private:
    int bias = 0;
    int params_num = 0;
    unordered_map<koopa_raw_value_t, int> var2bias;
}; 

stack st;

const vector<string> op_name = {
    "", "", "sgt", "slt", "", "", "add", "sub", "mul", "div", "rem", 
    "and", "or", "xor", "sll", "srl", "sra"
};

void visit_koopa(const koopa_raw_program_t& raw, ofstream& fout) {
    for (size_t i = 0; i < raw.values.len; ++i) {
        visit_koopa(reinterpret_cast<koopa_raw_value_t>(raw.values.buffer[i]), fout);
    }
    for (size_t i = 0; i < raw.funcs.len; ++i) {
        visit_koopa(reinterpret_cast<koopa_raw_function_t>(raw.funcs.buffer[i]), fout);
    }
}

void visit_koopa(const koopa_raw_function_t& func, ofstream& fout) {
    if (func->bbs.len == 0) {
        return;
    }
    string name(func->name + 1);
    riscv_printer.print("  .text", fout);
    riscv_printer.print("  .globl " + name, fout);
    riscv_printer.print(name + ":", fout);
    int num = st.precompute(func);
    if (num != 0) {
        riscv_printer.print_add(-num, fout);
    }
    if (flag_ra) {
        riscv_printer.print_ra("sw", num - 4, fout);
    }
    for (size_t i = 0; i < func->bbs.len; ++i) {
        visit_koopa(reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]), fout);
    }
    st.clear();
}

void visit_koopa(const koopa_raw_basic_block_t& bb, ofstream& fout) {
    if (strcmp(bb->name, "%entry")) {
        riscv_printer.print(string(bb->name + 1) + ":", fout);
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
            riscv_printer.print_sw("t0", st.getbias(value), fout);
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_LOAD:
            visit_koopa(value->kind.data.load, fout);
            riscv_printer.print_sw("t0", st.getbias(value), fout);
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
        case KOOPA_RVT_CALL:
            visit_koopa(value->kind.data.call, fout);
            if (value->kind.data.call.callee->ty->data.function.ret->tag == KOOPA_RTT_INT32) {
                riscv_printer.print_sw("a0", st.getbias(value), fout);
            }
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            visit_global(value, fout);
            break;
        default:
            assert(false);
    }
}

void visit_global(const koopa_raw_value_t& value, ofstream& fout) {
    string name = (value->name + 1);
    riscv_printer.print("  .data", fout);
    riscv_printer.print("  .globl " + name, fout);
    riscv_printer.print(name + ":", fout);
    auto init = value->kind.data.global_alloc.init;
    if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        riscv_printer.print("  .zero 4", fout);
    } else {
        riscv_printer.print("  .word " + to_string(init->kind.data.integer.value), fout);
    }
}

void visit_koopa(const koopa_raw_call_t& call, ofstream& fout) {
    for (size_t i = 0; i < std::min(8, static_cast<int>(call.args.len)); ++i) {
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            riscv_printer.print_load_const("a" + to_string(i), arg->kind.data.integer.value, fout);
        } else {
            riscv_printer.print_load("a" + to_string(i), st.getbias(arg), fout);
        }
    }
    for (size_t i = 8; i < call.args.len; ++i) {
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            riscv_printer.print_load_const("t0", arg->kind.data.integer.value, fout);
        } else {
            riscv_printer.print_load("t0", st.getbias(arg), fout);
        }
        riscv_printer.print_sw("t0", 4 * (i - 8), fout);
    }
    riscv_printer.print("  call " + string(call.callee->name + 1), fout);
}

void visit_koopa(const koopa_raw_jump_t& jump, ofstream& fout) {
    riscv_printer.print_jump(string(jump.target->name + 1), fout);
}

void visit_koopa(const koopa_raw_branch_t& branch, ofstream& fout) {
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t0", branch.cond->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(branch.cond), fout);
    }
    string tmplabel = "L_";
    tmplabel += to_string(tracktmp++);
    riscv_printer.print_branch(string(branch.true_bb->name + 1), string(branch.false_bb->name + 1), tmplabel, fout);
}

void visit_koopa(const koopa_raw_load_t& load, ofstream& fout) {
    if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        riscv_printer.print_load_global("t0", string(load.src->name + 1), fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(load.src), fout);
    }
}

void visit_koopa(const koopa_raw_store_t& store, ofstream& fout) {
    if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        int idx = store.value->kind.data.func_arg_ref.index;
        if (idx < 8) {
            if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                riscv_printer.print_sw_global("a" + to_string(idx), string(store.dest->name + 1), fout);
            } else {
                riscv_printer.print_sw("a" + to_string(idx), st.getbias(store.dest), fout);
            }
        } else {
            riscv_printer.print_load("t0", 4 * (idx - 8) + st.getnum(), fout);
            if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                riscv_printer.print_sw_global("t0", string(store.dest->name + 1), fout);
            } else {
                riscv_printer.print_sw("t0", st.getbias(store.dest), fout);
            }
        }
    } else {
        if (store.value->kind.tag == KOOPA_RVT_INTEGER) {
            riscv_printer.print_load_const("t0", store.value->kind.data.integer.value, fout);
        } else {
            riscv_printer.print_load("t0", st.getbias(store.value), fout);
        }
        if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            riscv_printer.print_sw_global("t0", string(store.dest->name + 1), fout);
        } else {
            riscv_printer.print_sw("t0", st.getbias(store.dest), fout);
        }
    } 
}

void visit_koopa(const koopa_raw_return_t& ret, ofstream& fout) {
    if (ret.value == nullptr) {
        if (flag_ra) {
            riscv_printer.print_ra("lw", st.getnum() - 4, fout);
        }

        riscv_printer.print_ret(st.getnum(), fout);
        return;
    }
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("a0", ret.value->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("a0", st.getbias(ret.value), fout);
    }
    if (flag_ra) {
        riscv_printer.print_ra("lw", st.getnum() - 4, fout);
    }
    riscv_printer.print_ret(st.getnum(), fout);
}

int visit_koopa(const koopa_raw_integer_t& integer, ofstream& fout) {
    return integer.value;
}

void visit_koopa(const koopa_raw_binary_t& binary, ofstream& fout) {
    koopa_raw_value_t lhs = binary.lhs, rhs = binary.rhs;
    if (lhs->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t0", lhs->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(lhs), fout);
    }
    if (rhs->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t1", rhs->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t1", st.getbias(rhs), fout);
    }
    switch (binary.op) {
        case KOOPA_RBO_EQ:
            riscv_printer.print_cmp("xor", "seqz", fout);
            break;
        case KOOPA_RBO_NOT_EQ:
            riscv_printer.print_cmp("xor", "snez", fout);
            break;
        case KOOPA_RBO_LE:
            riscv_printer.print_cmp("sgt", "seqz", fout);
            break;
        case KOOPA_RBO_GE:
            riscv_printer.print_cmp("slt", "seqz", fout);
            break;
        default:
            riscv_printer.print_op(op_name[static_cast<int>(binary.op)], fout);
            break;
    }
}
