#pragma once
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "ast.h"
#include "symbol.h"

using std::ofstream;
using std::vector;

class bs_cnt {
public:
    std::string getlabel(std::string label);
    std::string getlabel(std::string label, int num);
    int cnt = 0;
};

class Printer {
public:
    void print_binary(int idx, std::string& op, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_unary(int idx, std::string& op, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_load(int idx, std::string& ident, std::stringstream& out, SymbolTable& table);
    void print_store(bool flag, int idx, std::string& ident, std::stringstream& out, SymbolTable& table);
    void print_alloc(std::string& ident, std::stringstream& out, SymbolTable& table);
    void print_br(bool flag, int idx, std::string& label1, std::string& label2, std::stringstream& out);
    void print_jump(std::string& label, std::stringstream& out);
    void print_label(std::string& label, std::stringstream& out);
    void print_eq(bool flag, int idx, std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_lhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_rhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_decl(std::stringstream& out);
    void print_global_alloc(std::string& ident, int num, std::stringstream& out, GlobalSymbolTable& table);
    void print_alloc_arr(std::string& ident, vector<int>& num, std::stringstream& out, SymbolTable& table);
    void print_global_alloc_arr(std::string& ident, vector<int>& num, std::stringstream& out, GlobalSymbolTable& table);
    void print_store_array_const(std::string& ident, vector<pair<int, bool>>& nums, int idx, int num, std::stringstream& out, SymbolTable& table, bool flag);
    void print_load_array_const(std::string& ident, int i, int idx, std::stringstream& out, SymbolTable& table);
    void print_aggregate_const(std::vector<int>& data, vector<int>& num, std::stringstream& out);
    void print_store_array_var(std::string& ident, vector<pair<int, bool>>& nums, int idx, int num, std::stringstream& out, SymbolTable& table, bool flag);
    void print_aggregate_var(std::vector<pair<int, bool>>& data, std::vector<int>& nums, std::stringstream& out);
    void print_load_array(std::string& ident, vector<pair<int, bool>>& nums, int idx, std::stringstream& out, SymbolTable& table, bool flag);
    void print_init_arr_const(std::string& ident, vector<int>& data, vector<int>& nums, int& idx, std::stringstream& out, SymbolTable& table);
    void print_init_arr_var(std::string& ident, vector<pair<int, bool>>& data, vector<int>& nums, int& idx, std::stringstream& out, SymbolTable& table);
    void print_funcfparam(std::string& ident, vector<int>& nums, std::stringstream& out, SymbolTable& table);
    void print_funcfparam(std::string& ident, std::stringstream& out, SymbolTable& table, bool flag);
    void recursive_print(std::vector<int>& nums, int i, std::stringstream& out);
private:
    void recursive_print_agg_const(vector<int>& data, vector<int>& nums, int& i, int j , std::stringstream& out);
    void recursive_print_init_arr_const(std::string& ident, vector<int>& data, vector<int>& nums, int& idx, std::stringstream& out, 
                                        SymbolTable& table, int& i, int j);
    void recursive_print_agg_var(std::vector<pair<int, bool>>& data, vector<int>& nums, int& i, int j, std::stringstream& out);
    void recursive_print_init_arr_var(std::string& ident, vector<pair<int, bool>>& data, vector<int>& nums, int& idx, std::stringstream& out, 
                                        SymbolTable& table, int& i, int j);
    bs_cnt count;
};

class RiscvPrinter {
public:
    void print(string in, ofstream& out);
    void print_add(int num, ofstream& out);
    void print_sw(string op, int num, ofstream& out);
    void print_jump(string label, ofstream& out);
    void print_load_const(string dst, int num, ofstream& out);
    void print_load(string dst, int num, ofstream& out);
    void print_branch(string true_block, string false_block, string temp, ofstream& out);
    void print_cmp(string op1, string op2, ofstream& out);
    void print_op(string op, ofstream& out);
    void print_ret(int num, ofstream& out);
    void print_ra(string op, int num, ofstream& out);
    void print_load_global(string dst, string ident, ofstream& out);
    void print_sw_global(string src, string ident, ofstream& out);
    void print_sw_ptr(string dst, int num, ofstream& out);
    void print_load_ptr(string dst, int num, ofstream& out);
};