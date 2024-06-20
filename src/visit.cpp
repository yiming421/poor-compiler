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

    int precompute(const koopa_raw_function_t& func) { //precompute the bias of each variable
        for (size_t i = 0; i < func->bbs.len; ++i) {
            assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
            koopa_raw_basic_block_t bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
            for (size_t j = 0; j < bb->insts.len; ++j) {
                koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
                if (value->kind.tag == KOOPA_RVT_BINARY) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_ALLOC) {
                    size_t size = getsize(value->ty->data.pointer.base); // get the size of the variable
                    var2bias[value] = bias;
                    bias += size;
                } else if (value->kind.tag == KOOPA_RVT_LOAD) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_CALL) {
                    var2bias[value] = bias;
                    bias += 4;
                    flag_ra = true;
                    if (value->kind.data.call.args.len > 8) { // if the number of parameters is greater than 8
                    // then the parameters are stored in the stack
                        params_num = std::max(params_num, static_cast<int>(value->kind.data.call.args.len));
                    }
                } else if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
                    var2bias[value] = bias;
                    bias += 4;
                } else if (value->kind.tag == KOOPA_RVT_GET_PTR) {
                    var2bias[value] = bias;
                    bias += 4;
                }
            }
        }
        if (flag_ra) {
            bias += 4;
        } // if the function has a return value, then the return value is stored in the stack
        if (params_num > 8) {
            bias += 4 * (params_num - 8);
            for (auto& pair: var2bias) {
                var2bias[pair.first] += 4 * (params_num - 8); // update the bias of the parameters
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

size_t getsize(const koopa_raw_type_t& type) {
    if (type->tag == KOOPA_RTT_INT32) {
        return 4;
    } else if (type->tag == KOOPA_RTT_POINTER) {
        return 4;
    } else if (type->tag == KOOPA_RTT_ARRAY) {
        return getsize(type->data.array.base) * type->data.array.len;
    } else {
        return 0;
    }
}

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
    int num = st.precompute(func); // precompute the bias of each variable
    if (num != 0) {
        riscv_printer.print_add(-num, fout); // allocate space for the stack
    }
    if (flag_ra) {
        riscv_printer.print_ra("sw", num - 4, fout); // store the return address
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
            riscv_printer.print_sw("t0", st.getbias(value), fout); // store the result of the binary operation
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_LOAD:
            visit_koopa(value->kind.data.load, fout);
            riscv_printer.print_sw("t0", st.getbias(value), fout); // store the result of the load operation
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
                riscv_printer.print_sw("a0", st.getbias(value), fout); // store the return value
            }
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            visit_global(value, fout);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            visit_koopa(value->kind.data.get_elem_ptr, fout);
            riscv_printer.print_sw("t0", st.getbias(value), fout); // store the result of the get element pointer operation
            break;
        case KOOPA_RVT_GET_PTR:
            visit_koopa(value->kind.data.get_ptr, fout);
            riscv_printer.print_sw("t0", st.getbias(value), fout); // store the result of the get pointer operation
            break;
        default:
            assert(false);
    }
}

void visit_koopa(const koopa_raw_get_elem_ptr_t& gep, ofstream& fout) {
    if (gep.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        riscv_printer.print("  la t0, " + string(gep.src->name + 1), fout);
    } else if (gep.src->kind.tag == KOOPA_RVT_ALLOC) {
        riscv_printer.print("  li t0, " + to_string(st.getbias(gep.src)), fout);
        riscv_printer.print("  add t0, sp, t0", fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(gep.src), fout);
    } // load the address of the source variable
    if (gep.index->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t1", gep.index->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t1", st.getbias(gep.index), fout);
    } // load the index
    size_t size = getsize(gep.src->ty->data.pointer.base->data.array.base);
    riscv_printer.print("  li t2, " + to_string(size), fout);
    riscv_printer.print("  mul t1, t1, t2", fout);
    riscv_printer.print("  add t0, t0, t1", fout); // calculate the address of the element
}

void visit_koopa(const koopa_raw_get_ptr_t& getptr, ofstream& fout) {
    if (getptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        riscv_printer.print("  la t0, " + string(getptr.src->name + 1), fout);
    } else if (getptr.src->kind.tag == KOOPA_RVT_ALLOC) {
        riscv_printer.print("  li t0, " + to_string(st.getbias(getptr.src)), fout);
        riscv_printer.print("  add t0, sp, t0", fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(getptr.src), fout);
    } // load the address of the source variable
    if (getptr.index->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t1", getptr.index->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t1", st.getbias(getptr.index), fout);
    } // load the index
    size_t size = getsize(getptr.src->ty->data.pointer.base); // ???
    riscv_printer.print("  li t2, " + to_string(size), fout);
    riscv_printer.print("  mul t1, t1, t2", fout);
    riscv_printer.print("  add t0, t0, t1", fout);

}

void visit_global(const koopa_raw_value_t& value, ofstream& fout) {
    string name = (value->name + 1);
    riscv_printer.print("  .data", fout);
    riscv_printer.print("  .globl " + name, fout);
    riscv_printer.print(name + ":", fout);
    if (value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) { // if the variable is an integer
        auto init = value->kind.data.global_alloc.init;
        if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
            riscv_printer.print("  .zero 4", fout);
        } else {
            riscv_printer.print("  .word " + to_string(init->kind.data.integer.value), fout);
        }
    } else { // if the variable is an array
        auto init = value->kind.data.global_alloc.init;
        if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
            riscv_printer.print("  .zero " + to_string(getsize(value->ty->data.pointer.base)), fout);
        } else {
            int cnt = 0;
            print_init(init, fout, cnt);
            if (cnt != 0) {
                riscv_printer.print("  .zero " + to_string(cnt * 4), fout);
            }
        }
    }
}

void print_init(const koopa_raw_value_t& init, ofstream& fout, int& cnt) {
    if (init->kind.tag == KOOPA_RVT_INTEGER) {
        int num = init->kind.data.integer.value;
        if (num != 0) {
            if (cnt != 0) {
                riscv_printer.print("  .zero " + to_string(cnt * 4), fout);
            }
            riscv_printer.print("  .word " + to_string(init->kind.data.integer.value), fout);
            cnt = 0;
        } else {
            cnt++;
        }
    } else {
        for (size_t i = 0; i < init->kind.data.aggregate.elems.len; ++i) {
            print_init(reinterpret_cast<koopa_raw_value_t>(init->kind.data.aggregate.elems.buffer[i]), fout, cnt);
        }
    }
}

void visit_koopa(const koopa_raw_call_t& call, ofstream& fout) {
    for (size_t i = 0; i < std::min(8, static_cast<int>(call.args.len)); ++i) { // load the first 8 parameters
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            riscv_printer.print_load_const("a" + to_string(i), arg->kind.data.integer.value, fout);
        } else {
            riscv_printer.print_load("a" + to_string(i), st.getbias(arg), fout);
        }
    }
    for (size_t i = 8; i < call.args.len; ++i) { // load the rest of the parameters
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
    } else if (load.src->kind.tag == KOOPA_RVT_ALLOC) {
        riscv_printer.print_load("t0", st.getbias(load.src), fout);
    } else {
        riscv_printer.print_load_ptr("t0", st.getbias(load.src), fout);
    }
}

void visit_koopa(const koopa_raw_store_t& store, ofstream& fout) {
    if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) { // if the value is a function argument
        int idx = store.value->kind.data.func_arg_ref.index;
        if (idx < 8) { // if the index is less than 8, then the parameter is stored in the register
            if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                riscv_printer.print_sw_global("a" + to_string(idx), string(store.dest->name + 1), fout);
            } else if (store.dest->kind.tag == KOOPA_RVT_ALLOC) {
                riscv_printer.print_sw("a" + to_string(idx), st.getbias(store.dest), fout);
            } else {
                riscv_printer.print_sw_ptr("a" + to_string(idx), st.getbias(store.dest), fout);
            }
        } else { // if the index is greater than 8, then the parameter is stored in the stack
            riscv_printer.print_load("t0", 4 * (idx - 8) + st.getnum(), fout);
            if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                riscv_printer.print_sw_global("t0", string(store.dest->name + 1), fout);
            } else if (store.dest->kind.tag == KOOPA_RVT_ALLOC) {
                riscv_printer.print_sw("t0", st.getbias(store.dest), fout);
            } else {
                riscv_printer.print_sw_ptr("t0", st.getbias(store.dest), fout);
            }
        }
    } else { // if the value is not a function argument
        if (store.value->kind.tag == KOOPA_RVT_INTEGER) {
            riscv_printer.print_load_const("t0", store.value->kind.data.integer.value, fout);
        } else {
            riscv_printer.print_load("t0", st.getbias(store.value), fout);
        }
        if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            riscv_printer.print_sw_global("t0", string(store.dest->name + 1), fout);
        } else if (store.dest->kind.tag == KOOPA_RVT_ALLOC) {
            riscv_printer.print_sw("t0", st.getbias(store.dest), fout);
        } else {
            riscv_printer.print_sw_ptr("t0", st.getbias(store.dest), fout);
        }
    } 
}

void visit_koopa(const koopa_raw_return_t& ret, ofstream& fout) {
    if (ret.value == nullptr) { // if the function has no return value
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
