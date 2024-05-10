#pragma once
#include <memory>
#include <iostream>
#include <sstream>
#include "ast.h"
#include "symbol.h"


class Printer {
public:
    void print_binary(int idx, std::string op, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_lor(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_land(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_unary(int idx, std::string op, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_load(int idx, std::string ident, std::stringstream& out, SymbolTable& table);
    void print_store(bool flag, int idx, std::string ident, std::stringstream& out, SymbolTable& table);
    void print_alloc(std::string ident, std::stringstream& out, SymbolTable& table);
private:
    void print_lhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_rhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
};