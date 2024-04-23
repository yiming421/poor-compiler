#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include "ast.h"
#include "printer.h"

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

void Printer::print_binary(int idx, std::string op, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, 
                           std::stringstream& out) {
    out << "  %" << idx << " = " << op2str[op] << " ";
    print_lhs(lhs, out);
    print_rhs(rhs, out);
}

void Printer::print_lor(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out) {
    out << "  %" << idx << " = or ";
    print_lhs(lhs, out);
    print_rhs(rhs, out);
    out << "  %" << idx + 1 << " = ne 0, %" << idx << std::endl;
}

void Printer::print_land(int idx, std::unique_ptr<BaseAst>& lhs, std::unique_ptr<BaseAst>& rhs, std::stringstream& out) {
    out << "  %" << idx << " = ne 0, ";
    print_rhs(lhs, out);
    out << "  %" << idx + 1 << " = ne 0, ";
    print_rhs(lhs, out);
    out << "  %" << idx + 2 << " = and %" << idx << ", %" << idx + 1 << std::endl;
}

void Printer::print_unary(int idx, std::string op, std::unique_ptr<BaseAst>& rhs, std::stringstream& out) {
    if (op == "-") {
        out << "  %" << idx << " = sub 0, ";
    } else if (op == "!") {
        out << "  %" << idx << " = eq 0, ";
    }
    print_rhs(rhs, out);
}