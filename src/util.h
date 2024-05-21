#pragma once
#include <memory>
#include <iostream>
#include <sstream>
#include "ast.h"
#include "symbol.h"

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
private:
    void print_lhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_rhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    bs_cnt count;
};