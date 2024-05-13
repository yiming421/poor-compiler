#include "ast.h"
#include "util.h"
#include "symbol.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <cassert>

int BaseAst::id = 0;

Printer printer;
SymbolTable table;
bool end = false;
bs_cnt count;
 
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
    if (blockitem_list == nullptr) {
        return;
    }
    if (flag) {
        out << "{" << std::endl;
        out << "%entry:" << std::endl;
        blockitem_list->dump(out);
        out << "}" << std::endl;
    } else {
        blockitem_list->dump(out);
    }
}

void BlockItemListAst::dump(std::stringstream& out) {
    blockitem->dump(out);
    if (blockitem_list != nullptr && !end) {
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
    switch (type) {
        case 0: {
            exp->dump(out);
            count.cnt++;
            string then_label = count.getlabel("then");
            string end_label = count.getlabel("end");
            if (exp->idx == -1) {
                printer.print_br(false, exp->num, then_label, end_label, out);
            } else {
                printer.print_br(true, exp->idx, then_label, end_label, out);
            }
            printer.print_label(then_label, out);
            stmt->dump(out);
            if (!end) {
                printer.print_jump(end_label, out);
            } else {
                end = false;
            }
            printer.print_label(end_label, out);
            break;
        }
        case 1: {
            exp->dump(out);
            count.cnt++;
            string then_label = count.getlabel("then");
            string else_label = count.getlabel("else");
            string end_label = count.getlabel("end");
            if (exp->idx == -1) {
                printer.print_br(false, exp->num, then_label, else_label, out);
            } else {
                printer.print_br(true, exp->idx, then_label, else_label, out);
            }
            printer.print_label(then_label, out);
            with_else->dump(out);
            bool flag1 = end;
            if (!end) {
                printer.print_jump(end_label, out);
            } else {
                end = false;
            }
            printer.print_label(else_label, out);
            stmt->dump(out);
            bool flag2 = end;
            if (!end) {
                printer.print_jump(end_label, out);
            } else {
                end = false;
            }
            if (flag1 && flag2) {
                end = true;
            } else {
                printer.print_label(end_label, out);
            }
            break;
        }
        default:
            other_stmt->dump(out);
            break;
    }
}

void WithElseAst::dump(std::stringstream& out) {
    if (other_stmt != nullptr) {
        other_stmt->dump(out);
    } else {
        exp->dump(out);
        count.cnt++;
        string then_label = count.getlabel("then");
        string else_label = count.getlabel("else");
        string end_label = count.getlabel("end");
        if (exp->idx == -1) {
            printer.print_br(false, exp->num, then_label, else_label, out);
        } else {
            printer.print_br(true, exp->idx, then_label, else_label, out);
        }
        printer.print_label(then_label, out);
        if_withelse->dump(out);
        bool flag1 = end;
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(else_label, out);
        else_withelse->dump(out);
        bool flag2 = end;
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        if (flag1 && flag2) {
            end = true;
        } else {
            printer.print_label(end_label, out);
        }
    }
}

void OtherStmtAst::dump(std::stringstream& out) {
    switch (type) {
        case 0:
            exp->dump(out);
            if (exp->idx == -1) {
                out << "  ret " << exp->num << std::endl;
            } else {
                out << "  ret %" << exp->idx << std::endl;
            }
            end = true;
            break;
        case 1:
            idx = BaseAst::id;
            printer.print_load(idx, lval->ident, out, table);
            BaseAst::id++;
            exp->dump(out);
            if (exp->idx == -1) {
                printer.print_store(false, exp->num, lval->ident, out, table);
            } else {
                printer.print_store(true, exp->idx, lval->ident, out, table);
            }
            break;
        case 2:
            table.push();
            block->dump(out);
            table.pop();
            break;
        case 3:
            exp->dump(out);
            break;
        default:
            break;
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
            printer.print_load(idx, lval->ident, out, table);
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
    printer.print_alloc(ident, out, table);
    if (initval != nullptr) {
        initval->dump(out);
        if (initval->idx == -1) {
            printer.print_store(false, initval->num, ident, out, table);
        } else {
            printer.print_store(true, initval->idx, ident, out, table);
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