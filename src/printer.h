#pragma once
#include <memory>
#include <iostream>
#include <sstream>
#include "ast.h"


class Printer {
public:
    void print_binary(int idx, std::string op, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_lor(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_land(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
    void print_unary(int idx, std::string op, std::unique_ptr<BaseAst>& rhs, std::stringstream& out);
private:
    void print_lhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
    void print_rhs(std::unique_ptr<BaseAst>& ptr, std::stringstream& out);
};