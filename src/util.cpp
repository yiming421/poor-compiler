#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <cassert>
#include <fstream>
#include "ast.h"
#include "util.h"
#include "symbol.h"

using std::ofstream;
using std::endl;
using std::string;

std::unordered_map<std::string, std::string> op2str = {
    {"+", "add"},
    {"-", "sub"},
    {"*", "mul"},
    {"/", "div"},
    {"%", "mod"},
    {"==", "eq"},
    {"!=", "ne"},
    {"<", "lt"},
    {"<=", "le"},
    {">", "gt"},
    {">=", "ge"}
};

std::string bs_cnt::getlabel(std::string label) {
    return label + "_" + std::to_string(cnt);
}

std::string bs_cnt::getlabel(std::string label, int num) {
    return label + "_" + std::to_string(num);
}

void Printer::print_lhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out) {
    if (ptr->idx >= 0) {
        out << "%" << ptr->idx << ", ";
    } else if (ptr->idx == -1) {
        out << ptr->num << ", ";
    } else {
        out << ptr->ident << ", ";
    }
}

void Printer::print_rhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out) {
    if (ptr->idx >= 0) {
        out << "%" << ptr->idx << std::endl;
    } else if (ptr->idx == -1) {
        out << ptr->num << std::endl;
    } else {
        out << ptr->ident << std::endl;
    }
}

void Printer::print_binary(int idx, std::string& op, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, 
                           std::stringstream& out) {
    out << "  %" << idx << " = " << op2str[op] << " ";
    print_lhs(lhs, out);
    print_rhs(rhs, out);
}

void Printer::print_unary(int idx, std::string& op, std::unique_ptr<BaseAst>& rhs, std::stringstream& out) {
    if (op == "-") {
        out << "  %" << idx << " = sub 0, ";
    } else if (op == "!") {
        out << "  %" << idx << " = eq 0, ";
    }
    print_rhs(rhs, out);
}

void Printer::print_load(int idx, std::string& ident, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    int len = table.get(ident);
    int flag = table.isConst(ident);
    if (len >= 0 || flag) {
        out << "  %" << idx << " = load @" << (ident + '_' + std::to_string(id)) << std::endl;
    } else {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id))  << ", 0" << std::endl;
    }
}

void Printer::print_store(bool flag, int idx, std::string& ident, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    if (flag) {
        out << "  store %" << idx << ", @" << (ident + '_' + std::to_string(id)) << std::endl;
    } else {
        out << "  store " << idx << ", @" << ident + '_' + std::to_string(id) << std::endl;
    }
}

void Printer::print_alloc(std::string& ident, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    out << "  @" << (ident + '_' + std::to_string(id)) << " = alloc i32" << std::endl;
}

void Printer::print_br(bool flag, int idx, std::string& label1, std::string& label2, std::stringstream& out) {
    if (flag) {
        out << "  br %" << idx << ", %" << label1 << ", %" << label2 << std::endl;
    } else {
        out << "  br " << idx << ", %" << label1 << ", %" << label2 << std::endl;
    }
}

void Printer::print_jump(std::string& label, std::stringstream& out) {
    out << "  jump %" << label << std::endl;
}

void Printer::print_label(std::string& label, std::stringstream& out) {
    out << "%" << label << ":" << std::endl;
}

void Printer::print_eq(bool flag, int idx, std::unique_ptr<BaseAst>& ptr, std::stringstream& out) {
    if (flag) {
        out << "  %" << idx << " = eq 0, ";
    } else {
        out << "  %" << idx << " = ne 0, ";
    }
    print_rhs(ptr, out);
}

void Printer::print_decl(std::stringstream& out) {
    out << "decl @getint(): i32" << std::endl;
    out << "decl @getch(): i32" << std::endl;
    out << "decl @getarray(*i32): i32" << std::endl;
    out << "decl @putint(i32)" << std::endl;
    out << "decl @putch(i32)" << std::endl;
    out << "decl @putarray(i32, *i32)" << std::endl;
    out << "decl @starttime()" << std::endl;
    out << "decl @stoptime()" << std::endl;
}

void Printer::print_global_alloc(std::string& ident, int num, std::stringstream& out, GlobalSymbolTable& table) {
    out << "global @" << ident + '_' + std::to_string(0) << " = alloc i32, ";
    if (num == 0) {
        out << "zeroinit" << std::endl;
    } else {
        out << num << std::endl;
    }
}

void Printer::recursive_print(std::vector<int>& nums, int i, std::stringstream& out) {
    if (i == nums.size()) {
        out << "i32";
        return;
    }
    out << '[';
    recursive_print(nums, i + 1, out);
    out << ", " << nums[i] << ']';
}

void Printer::print_alloc_arr(std::string& ident, vector<int>& nums, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    out << "  @" << (ident + '_' + std::to_string(id)) << " = alloc ";
    recursive_print(nums, 0, out);
} 

void Printer::print_global_alloc_arr(std::string& ident, vector<int>& num, std::stringstream& out, GlobalSymbolTable& table) {
    out << "global @" << ident + '_' + std::to_string(0) << " = alloc ";
    recursive_print(num, 0, out);
}

void Printer::recursive_print_agg_const(vector<int>& data, vector<int>& nums, int& i, int j , std::stringstream& out) {
    if (j == nums.size() - 1) {
        out << '{';
        for (int k = 0; k < nums[j]; k++) {
            if (k != 0) {
                out << ", ";
            }
            out << data[i++];
        }
        out << '}';
        return;
    }
    out << '{';
    for (int k = 0; k < nums[j]; k++) {
        if (k != 0) {
            out << ", ";
        }
        recursive_print_agg_const(data, nums, i, j + 1, out);
    }
    out << '}';
}

void Printer::print_aggregate_const(vector<int>& data, vector<int>& nums, std::stringstream& out) {
    out << ", ";
    int i = 0, j = 0;
    recursive_print_agg_const(data, nums, i, j, out);
    out << std::endl;
}

void Printer::print_aggregate_var(std::vector<pair<int, bool>>& data, vector<int>& nums, std::stringstream& out) {
    out << ", ";
    int i = 0, j = 0;
    recursive_print_agg_var(data, nums, i, j, out);
    out << std::endl;
}

void Printer::recursive_print_agg_var(std::vector<pair<int, bool>>& data, vector<int>& nums, int& i, int j, std::stringstream& out) {
    if (j == nums.size() - 1) {
        out << '{';
        for (int k = 0; k < nums[j]; k++) {
            if (k != 0) {
                out << ", ";
            }
            if (data[i].second) {
                out << '%' << data[i++].first;
            } else {
                out << data[i++].first;
            }
        }
        out << '}';
        return;
    }
    out << '{';
    for (int k = 0; k < nums[j]; k++) {
        if (k != 0) {
            out << ", ";
        }
        recursive_print_agg_var(data, nums, i, j + 1, out);
    }
    out << '}';
}

void Printer::recursive_print_init_arr_const(std::string& ident, vector<int>& data, vector<int>& nums, int& idx, std::stringstream& out, 
                                       SymbolTable& table, int& i, int j) {
    if (j == nums.size() - 1) {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx  << ", " << k << std::endl;
            out << "  store " << data[i++] << ", %" << idx << std::endl;
            idx += 1;
        }
        return;
    } else {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx << ", " << k << std::endl;
            idx += 1;
            recursive_print_init_arr_const(ident, data, nums, idx, out, table, i, j + 1);
        }
    }
}

void Printer::print_init_arr_const(std::string& ident, vector<int>& data, vector<int>& nums, int& idx, std::stringstream& out, SymbolTable& table) {
    int i = 0;
    for (int k = 0; k < nums[0]; k++) {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(table.getID(ident))) << ", " << k << std::endl;
        if(nums.size() == 1) {
            out << "  store " << data[i++] << ", %" << idx << std::endl;
            idx += 1;
        } else {
            idx += 1;
            recursive_print_init_arr_const(ident, data, nums, idx, out, table, i, 1);
        }
    }
}

void Printer::recursive_print_init_arr_var(std::string& ident, vector<pair<int, bool>>& data, vector<int>& nums, int& idx, std::stringstream& out, 
                                     SymbolTable& table, int& i, int j) {
    if (j == nums.size() - 1) {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx  << ", " << k << std::endl;
            if (data[i].second) {
                out << "  store %" << data[i++].first << ", %" << idx << std::endl;
            } else {
                out << "  store " << data[i++].first << ", %" << idx << std::endl;
            }
            idx += 1;
        }
        return;
    } else {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx << ", " << k << std::endl;
            idx += 1;
            recursive_print_init_arr_var(ident, data, nums, idx, out, table, i, j + 1);
        }
    }
}

void Printer::print_init_arr_var(std::string& ident, vector<pair<int, bool>>& data, vector<int>& nums, int& idx, std::stringstream& out, SymbolTable& table) {
    int i = 0;
    for (int k = 0; k < nums[0]; k++) {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(table.getID(ident))) << ", " << k << std::endl;
        if (nums.size() == 1) {
            if (data[i].second) {
                out << "  store %" << data[i++].first << ", %" << idx << std::endl;
            } else {
                out << "  store " << data[i++].first << ", %" << idx << std::endl;
            }
            idx += 1;
        } else {
            idx += 1;
            recursive_print_init_arr_var(ident, data, nums, idx, out, table, i, 1);
        }
    }
}

void Printer::print_store_array_const(string& ident, vector<pair<int, bool>>& nums, int idx, int num, std::stringstream& out, SymbolTable& table, bool flag) {
    int id = table.getID(ident);
    if (flag) {
        out << "  %" << idx << " = load @" << (ident + '_' + std::to_string(id)) << std::endl;
        out << "  %" << idx + 1 << " = getptr %" << idx << ", ";
        idx += 1;
    } else {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id)) << ", ";
    }
    if (nums[0].second) {
        out << "%" << nums[0].first << std::endl;
    } else {
        out << nums[0].first << std::endl;
    }
    for (int i = 1; i < nums.size(); i++) {
        idx += 1;
        out << "  %" << idx << " = getelemptr %" << idx - 1 << ", ";
        if (nums[i].second) {
            out << "%" << nums[i].first << std::endl;
        } else {
            out << nums[i].first << std::endl;
        }
    }
    out << "  store " << num << ", %" << idx << std::endl;
}

void Printer::print_load_array_const(string& ident, int i, int idx, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id)) << ", " << i << std::endl;
    out << "  %" << idx + 1 << " = load %" << idx << std::endl;
}

void Printer::print_store_array_var(std::string& ident, vector<pair<int, bool>>& nums, int idx, int num, std::stringstream& out, SymbolTable& table, bool flag) {
    int id = table.getID(ident);
    if (flag) {
        out << "  %" << idx << " = load @" << (ident + '_' + std::to_string(id)) << std::endl;
        out << "  %" << idx + 1 << " = getptr %" << idx << ", ";
        idx += 1;
    } else {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id)) << ", ";
    }
    if (nums[0].second) {
        out << "%" << nums[0].first << std::endl;
    } else {
        out << nums[0].first << std::endl;
    }
    for (int i = 1; i < nums.size(); i++) {
        idx += 1;
        out << "  %" << idx << " = getelemptr %" << idx - 1 << ", ";
        if (nums[i].second) {
            out << "%" << nums[i].first << std::endl;
        } else {
            out << nums[i].first << std::endl;
        }
    }
    out << "  store %" << num << ", %" << idx << std::endl;
}

void Printer::print_load_array(std::string& ident, vector<pair<int, bool>>& nums, int idx, std::stringstream& out, SymbolTable& table, bool flag) {
    int id = table.getID(ident);
    if (flag) {
        out << "  %" << idx << " = load @" << (ident + '_' + std::to_string(id)) << std::endl;
        out << "  %" << idx + 1 << " = getptr %" << idx << ", ";
        idx += 1;
    } else {
        out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id)) << ", ";
    }
    if (nums[0].second) {
        out << "%" << nums[0].first << std::endl;
    } else {
        out << nums[0].first << std::endl;
    }
    for (int i = 1; i < nums.size(); i++) {
        idx += 1;
        out << "  %" << idx << " = getelemptr %" << idx - 1 << ", ";
        if (nums[i].second) {
            out << "%" << nums[i].first << std::endl;
        } else {
            out << nums[i].first << std::endl;
        }
    }
    int len = table.get(ident);
    if (abs(len) != nums.size()) {
        out << "  %" << idx + 1 << " = getelemptr %" << idx << ", 0" << std::endl;
    } else {
        out << "  %" << idx + 1 << " = load %" << idx << std::endl;
    }
}

void Printer::print_funcfparam(std::string& ident, std::stringstream& out, SymbolTable& table, bool flag) {
    out << "@" << ident + '_' + std::to_string(table.getID(ident) - 1) << ": ";
    if (flag) {
        out << "*i32";
    } else {
        out << "i32";
    }
}

void Printer::print_funcfparam(std::string& ident, vector<int>& nums, std::stringstream& out, SymbolTable& table) {
    out << "@" << ident + '_' + std::to_string(table.getID(ident) - 1) << ": *";
    recursive_print(nums, 0, out);
}

void RiscvPrinter::print(string in, ofstream& out) {
    out << in << endl;
}

void RiscvPrinter::print_add(int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add sp, sp, t2" << endl;
}

void RiscvPrinter::print_sw(string dst, int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add t2, sp, t2" << endl;
    out << "  sw " << dst << ", 0(t2)" << endl;
}

void RiscvPrinter::print_sw_ptr(string dst, int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add t2, sp, t2" << endl;
    out << "  lw t2, 0(t2)" << endl;
    out << "  sw " << dst << ", 0(t2)" << endl;
}

void RiscvPrinter::print_load_ptr(string dst, int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add t2, sp, t2" << endl;
    out << "  lw " << "t2" << ", 0(t2)" << endl;
    out << "  lw " << dst << ", 0(t2)" << endl;
}

void RiscvPrinter::print_jump(string label, ofstream& out) {
    out << "  j " << label << endl;
}

void RiscvPrinter::print_load_const(string dst, int num, ofstream& out) {
    out << "  li " << dst << ", " << num << endl;
}

void RiscvPrinter::print_load(string dst, int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add t2, sp, t2" << endl;
    out << "  lw " << dst << ", 0(t2)" << endl;
}

void RiscvPrinter::print_branch(string true_block, string false_block, string temp, ofstream& out) {
    out << "  bnez t0, " << temp << endl;
    out << "  j " << false_block << endl;
    out << temp << ":" << endl;
    out << "  j " << true_block << endl;
}

void RiscvPrinter::print_cmp(string op1, string op2, ofstream& out) {
    out << "  " << op1  << " t0, t0, t1" << endl;
    out << "  " << op2 << " t0, t0" << endl;
}

void RiscvPrinter::print_op(string op, ofstream& out) {
    out << "  " << op << " t0, t0, t1" << endl;
}

void RiscvPrinter::print_ret(int num, ofstream& out) {
    if (num != 0) {
        out << "  li t2, " << num << endl;
        out << "  add sp, sp, t2" << endl;
    }
    out << "  ret" << endl;
}

void RiscvPrinter::print_ra(string op, int num, ofstream& out) {
    out << "  li t2, " << num << endl;
    out << "  add t2, sp, t2" << endl;
    out << "  " << op << " ra, 0(t2)" << endl;
}

void RiscvPrinter::print_load_global(string dst, string ident, ofstream& out) {
    out << "  la " << dst << ", " << ident << endl;
    out << "  lw " << dst << ", 0(" << dst << ")" << endl;
}

void RiscvPrinter::print_sw_global(string src, string ident, ofstream& out) {
    out << "  la t1, " << ident << endl;
    out << "  sw " << src << ", 0(t1)" << endl;
}


