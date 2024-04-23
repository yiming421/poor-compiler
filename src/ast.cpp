#include "ast.h"
#include "printer.h"
#include "symbol.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <cassert>

int BaseAst::id = 0;

Printer printer;
SymbleTable table;
 
void CompUnitAst::dump(std::stringstream& out) {
    func_def->dump(out);
}

void FuncDefAst::dump(std::stringstream& out) {
    out << "fun @" << ident << "(): ";
    func_type->dump(out);
    block->dump(out);
}

void FuncTypeAst::dump(std::stringstream& out) {
    out << type << " ";
}

void BlockAst::dump(std::stringstream& out) {
    out << "{" << std::endl;
    out << "@entry:" << std::endl;
    blockitem_list->dump(out);
    out << "}" << std::endl;
}

void BlockItemListAst::dump(std::stringstream& out) {
    blockitem->dump(out);
    if (blockitem_list != nullptr) {
        blockitem_list->dump(out);
    }
}

void BlockItemAst::dump(std::stringstream& out) {
    if (stmt != nullptr) {
        stmt->dump(out);
    } else {
        decl->dump(out);
    }
}

void StmtAst::dump(std::stringstream& out) {
    if (lval == nullptr) {
        exp->dump(out);
        if (exp->idx == -1) {
            out << "  ret " << exp->num << std::endl;
        } else {
            out << "  ret %" << exp->idx << std::endl;
        }
    } else {
        idx = BaseAst::id;
        out << "  %" << idx << " = load @" << lval->ident << std::endl;
        BaseAst::id++;
        exp->dump(out);
        if (exp->idx == -1) {
            out << "  store " << exp->num << ", @" << lval->ident << std::endl;
        } else {
            out << "  store %" << exp->idx << ", @" << lval->ident << std::endl;
        }
    }
}

void ExpAst::dump(std::stringstream& out) {
    lor_exp->dump(out);
    idx = lor_exp->idx;
    num = lor_exp->num;
    ident = lor_exp->ident;
}

int ExpAst::cal() {
    return lor_exp->cal();
}

void PrimaryExpAst::dump(std::stringstream& out) {
    if (type == 1){
        idx = -1;
        num = number;
    } else if (type == 0){
        exp->dump(out);
        idx = exp->idx;
        num = exp->num;
    } else {
        assert(table.isExist(lval->ident));
        if (table.isConst(lval->ident)) {
            idx = -1;
            num = lval->cal();
        } else {
            idx = BaseAst::id;
            out << "  %" << idx << " = load @" << lval->ident << std::endl;
            BaseAst::id++;
        }
    }
}

int PrimaryExpAst::cal() {
    if (type == 1){
        return number;
    } else if (type == 0) {
        return exp->cal();
    } else {
        return lval->cal();
    }
}

void UnaryExpAst::dump(std::stringstream& out) {
    if (type != 0){
        unary_exp->dump(out);
        idx = BaseAst::id;
        if (op != "+"){
            printer.print_unary(idx, op, unary_exp, out);
            BaseAst::id++;
        } else {
            idx = unary_exp->idx;
            num = unary_exp->num;
        }
    } else {
        primary_exp->dump(out);
        idx = primary_exp->idx;
        num = primary_exp->num;
        ident = primary_exp->ident;
    }
}

int UnaryExpAst::cal() {
    if (type != 0){
        int num1 = unary_exp->cal();
        if (op == "-"){
            return -num1;
        } else {
            return !num1;
        }
    } else {
        return primary_exp->cal();
    }
}

void AddExpAst::dump(std::stringstream& out) {
    if (type != 0){
        add_exp->dump(out);
        mul_exp->dump(out);
        idx = BaseAst::id;
        printer.print_binary(idx, op, add_exp, mul_exp, out);
        BaseAst::id++;
    } else {
        mul_exp->dump(out);
        idx = mul_exp->idx;
        num = mul_exp->num;
        ident = mul_exp->ident;
    }
}

int AddExpAst::cal() {
    if (type != 0){
        int num1 = add_exp->cal();
        int num2 = mul_exp->cal();
        if (op == "+"){
            return num1 + num2;
        } else {
            return num1 - num2;
        }
    } else {
        return mul_exp->cal();
    }
}

void MulExpAst::dump(std::stringstream& out) {
    if (type != 0){
        mul_exp->dump(out);
        unary_exp->dump(out);
        idx = BaseAst::id;
        printer.print_binary(idx, op, mul_exp, unary_exp, out);
        BaseAst::id++;
    } else {
        unary_exp->dump(out);
        idx = unary_exp->idx;
        num = unary_exp->num;
        ident = unary_exp->ident;
    }
}

int MulExpAst::cal() {
    if (type != 0){
        int num1 = mul_exp->cal();
        int num2 = unary_exp->cal();
        if (op == "*"){
            return num1 * num2;
        } else if (op == "/"){
            return num1 / num2;
        } else {
            return num1 % num2;
        }
    } else {
        return unary_exp->cal();
    }
}

void RelExpAst::dump(std::stringstream& out) {
    if (type != 0){
        rel_exp->dump(out);
        add_exp->dump(out);
        idx = BaseAst::id;
        printer.print_binary(idx, op, rel_exp, add_exp, out);
        BaseAst::id++;
    } else {
        add_exp->dump(out);
        idx = add_exp->idx;
        num = add_exp->num;
        ident = add_exp->ident;
    }
}

int RelExpAst::cal() {
    if (type != 0){
        int num1 = rel_exp->cal();
        int num2 = add_exp->cal();
        if (op == "<"){
            return num1 < num2;
        } else if (op == "<="){
            return num1 <= num2;
        } else if (op == ">"){
            return num1 > num2;
        } else {
            return num1 >= num2;
        }
    } else {
        return add_exp->cal();
    }
}

void EqExpAst::dump(std::stringstream& out) {
    if (type != 0){
        eq_exp->dump(out);
        rel_exp->dump(out);
        idx = BaseAst::id;
        printer.print_binary(idx, op, eq_exp, rel_exp, out);
        BaseAst::id++;
    } else {
        rel_exp->dump(out);
        idx = rel_exp->idx;
        num = rel_exp->num;
        ident = rel_exp->ident;
    }
}

int EqExpAst::cal() {
    if (type != 0){
        int num1 = eq_exp->cal();
        int num2 = rel_exp->cal();
        if (op == "=="){
            return num1 == num2;
        } else {
            return num1 != num2;
        }
    } else {
        return rel_exp->cal();
    }
}

void LAndExpAst::dump(std::stringstream& out) {
    if (type != 0){
        land_exp->dump(out);
        eq_exp->dump(out);
        idx = BaseAst::id;
        printer.print_land(idx, land_exp, eq_exp, out);
        BaseAst::id += 3;
        idx += 2;
    } else {
        eq_exp->dump(out);
        idx = eq_exp->idx;
        num = eq_exp->num;
        ident = eq_exp->ident;
    }
}

int LAndExpAst::cal() {
    if (type != 0){
        int num1 = land_exp->cal();
        int num2 = eq_exp->cal();
        return num1 && num2;
    } else {
        return eq_exp->cal();
    }
}

void LOrExpAst::dump(std::stringstream& out) {
    if (type != 0){
        lor_exp->dump(out);
        land_exp->dump(out);
        idx = BaseAst::id;
        printer.print_lor(idx, lor_exp, land_exp, out);
        BaseAst::id += 2;
        idx += 1;
    } else {
        land_exp->dump(out);
        idx = land_exp->idx;
        num = land_exp->num;
        ident = land_exp->ident;
    }
}

int LOrExpAst::cal() {
    if (type != 0){
        int num1 = lor_exp->cal();
        int num2 = land_exp->cal();
        return num1 || num2;
    } else {
        return land_exp->cal();
    }
}

void DeclAst::dump(std::stringstream& out) {
    if (const_decl != nullptr) {
        const_decl->cal();
    } else {
        var_decl->dump(out);
    }
}

int ConstDeclAst::cal() {
    assert(const_def_list != nullptr);
    const_def_list->cal();
    return 0;
}

void VarDeclAst::dump(std::stringstream& out) {
    var_def_list->dump(out);
}

int ConstDefListAst::cal() {
    assert(const_def != nullptr);
    const_def->cal();
    if (const_def_list != nullptr) {
        const_def_list->cal();
    }
    return 0;
}

void VarDefListAst::dump(std::stringstream& out) {
    var_def->dump(out);
    if (var_def_list != nullptr) {
        var_def_list->dump(out);
    }
}

int ConstDefAst::cal() {
    assert(const_init_val != nullptr);
    int num = const_init_val->cal();
    table.insert(ident, num);
    return 0;
}

void VarDefAst::dump(std::stringstream& out) {
    table.insert(ident);
    out << "  @" << ident << " = " << "alloc i32" << std::endl;
    if (initval != nullptr) {
        initval->dump(out);
        if (initval->idx == -1) {
            out << "  store " << initval->num << ", @" << ident << std::endl;
        } else {
            out << "  store %" << initval->idx << ", @" << ident << std::endl;
        }
    }
}

int ConstInitValAst::cal() {
    assert(const_exp != nullptr);
    return const_exp->cal();
}

void InitValAst::dump(std::stringstream& out) {
    exp->dump(out);
    idx = exp->idx;
    num = exp->num;
}

int ConstExpAst::cal() {
    return exp->cal();
}

int LValAst::cal() {
    assert(table.isExist(ident) && table.isConst(ident));
    return table.get(ident);
}