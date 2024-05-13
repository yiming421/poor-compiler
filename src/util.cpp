#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <cassert>
#include "ast.h"
#include "util.h"
#include "symbol.h"

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
    if (label == "then") {
        return "then_" + std::to_string(cnt);
    } else if (label == "else") {
        return "else_" + std::to_string(cnt);
    } else if (label == "end") {
        return "end_" + std::to_string(cnt);
    }
    assert(false);
    return "";
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
    out << "  %" << idx << " = load @" << (ident + std::to_string(id)) << std::endl;
}

void Printer::print_store(bool flag, int idx, std::string& ident, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    if (flag) {
        out << "  store %" << idx << ", @" << (ident + std::to_string(id)) << std::endl;
    } else {
        out << "  store " << idx << ", @" << ident + std::to_string(id) << std::endl;
    }
}

void Printer::print_alloc(std::string& ident, std::stringstream& out, SymbolTable& table) {
    int id = table.getID(ident);
    out << "  @" << (ident + std::to_string(id)) << " = alloc i32" << std::endl;
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