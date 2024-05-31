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
GlobalSymbolTable& gst = table.gst;
bool end = false;
bs_cnt count;
bool flag_main = false;
bool flag_type = false;
 
void CompUnitAst::dump(std::stringstream& out) {
    printer.print_decl(out);
    comp_unit_list->dump(out);
    assert(flag_main);
}

void CompUnitListAst::dump(std::stringstream& out) {
    decl_or_def->dump(out);
    if (comp_unit_list != nullptr) {
        comp_unit_list->dump(out);
    }
}

void DeclOrDefAst::dump(std::stringstream& out) {
    if (decl != nullptr) {
        decl->idx = -1;
        decl->dump(out);
    } else {
        func_def->dump(out);
    }
}

void FuncDefAst::dump(std::stringstream& out) {
    table.clear();
    reinterpret_cast<BlockAst&>(*block).flag = true;   
    out << "fun @" << ident << "(";
    if (ident == "main") {
        flag_main = true;
    }
    vector<pair<string, string>> params;
    if (func_fparams != nullptr) {
        reinterpret_cast<FuncFparamsAst&>(*func_fparams).dump(out, params);
    }
    vector<string> pars;
    for (auto& param : params) {
        pars.push_back(param.first);
    }
    bool flag = false;
    if (func_type->ident == "i32") {
        flag = true;
    } else {
        flag = false;
    }
    assert(gst.insert_func(ident, pars, flag));
    out << ")";
    func_type->dump(out);
    out << " {" << std::endl;
    end = false;
    reinterpret_cast<BlockAst&>(*block).dump(out, params);
    if (!end) {
        if (flag_type) {
            out << "  ret 0" << std::endl;
        } else {
            out << "  ret" << std::endl;
        } 
    }
    out << "}" << std::endl;
}

void FuncFparamsAst::dump(std::stringstream& out, vector<pair<string, string>>& params) {
    func_fparam->dump(out);
    params.push_back(std::make_pair(reinterpret_cast<FuncFparamAst&>(*func_fparam).type, func_fparam->ident));
    if (func_fparams != nullptr) {
        out << ", ";
        reinterpret_cast<FuncFparamsAst&>(*func_fparams).dump(out, params);
    }
}

void FuncFparamAst::dump(std::stringstream& out) {
    table.insert(ident);
    out << "@" << ident + '_' + std::to_string(table.getID(ident) - 1) << ": " << type;
}

void BTypeAst::dump(std::stringstream& out) {
    if (ident == "i32") {
        out << ": i32";
        flag_type = true;
    } else {
        flag_type = false;
    }
}

void BlockAst::dump(std::stringstream& out) {
    if (blockitem_list == nullptr) {
        return;
    }
    if (flag) {
        out << "%entry:" << std::endl;
        blockitem_list->idx = idx;
        blockitem_list->dump(out);
    } else {
        blockitem_list->idx = idx;
        blockitem_list->dump(out);
    }
}

void BlockAst::dump(std::stringstream& out, vector<pair<string, string>>& params) {
    out << "%entry:" << std::endl;
    for (auto& param : params) {
        int num = table.getID(param.second);
        out << "  @" << param.second + '_' +std::to_string(num) << " = alloc i32" << std::endl;
        out << "  store @" << param.second + '_' + std::to_string(num - 1) << ", @" << param.second + '_' + std::to_string(num) << std::endl;
    }
    if (blockitem_list == nullptr) {
        return;
    }
    blockitem_list->dump(out);
}

void BlockItemListAst::dump(std::stringstream& out) {
    blockitem->idx = idx;
    blockitem->dump(out);
    if (blockitem_list != nullptr) {
        blockitem_list->idx = idx;
        blockitem_list->dump(out);
    }
}

void BlockItemAst::dump(std::stringstream& out) {
    if (stmt != nullptr) {
        stmt->idx = idx;
        stmt->dump(out);
    } else {
        decl->dump(out);
    }
}


void StmtAst::dump(std::stringstream& out) {
    if (end) {
        return;
    }
    if (if_stmt != nullptr) {
        if_stmt->idx = idx;
        if_stmt->dump(out);
    } else {
        with_else->idx = idx;
        with_else->dump(out);
    }
}

void IfStmtAst::dump(std::stringstream& out) {
    if (stmt != nullptr) {
        end = false;
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
        stmt->idx = idx;
        stmt->dump(out);
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(end_label, out);
    } else {
        end = false;
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
        with_else->idx = idx;
        with_else->dump(out);
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(else_label, out);
        if_stmt->idx = idx;
        if_stmt->dump(out);
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(end_label, out);
    }
}

void WithElseAst::dump(std::stringstream& out) {
    if (other_stmt != nullptr) {
        other_stmt->idx = idx;
        other_stmt->dump(out);
    } else {
        end = false;
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
        if_withelse->idx = idx;
        if_withelse->dump(out);
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(else_label, out);
        else_withelse->idx = idx;
        else_withelse->dump(out);
        if (!end) {
            printer.print_jump(end_label, out);
        } else {
            end = false;
        }
        printer.print_label(end_label, out);
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
            block->idx = idx;
            block->dump(out);
            table.pop();
            break;
        case 3:
            exp->dump(out);
            break;
        case 5: {
            end = false;
            count.cnt++;
            stmt->idx = count.cnt;
            string entry_label = count.getlabel("entry");
            string body_label = count.getlabel("body");
            string end_label = count.getlabel("end");
            printer.print_jump(entry_label, out);
            printer.print_label(entry_label, out);
            exp->dump(out);
            if (exp->idx == -1) {
                printer.print_br(false, exp->num, body_label, end_label, out);
            } else {
                printer.print_br(true, exp->idx, body_label, end_label, out);
            }
            printer.print_label(body_label, out);
            stmt->dump(out);
            if (!end) {
                printer.print_jump(entry_label, out);
            } else {
                end = false;
            }
            printer.print_label(end_label, out);
            break;
        }
        case 7: {
            assert(idx != 0);
            string end_label = count.getlabel("end", idx);
            printer.print_jump(end_label, out);
            end = true;
            break;
        }
        case 6: {
            assert(idx != 0);
            string entry_label = count.getlabel("entry", idx);
            printer.print_jump(entry_label, out);
            end = true;
            break;
        }
        case 8: 
            if (flag_type) {
                out << "  ret 0" << std::endl;
            } else {
                out << "  ret" << std::endl;
            }
            end = true;
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
    if (type == 1){
        unary_exp->dump(out);
        idx = BaseAst::id;
        if (op != "+"){
            printer.print_unary(idx, op, unary_exp, out);
            BaseAst::id++;
        } else {
            idx = unary_exp->idx;
            num = unary_exp->num;
        }
    } else if (type == 0) {
        primary_exp->dump(out);
        idx = primary_exp->idx;
        num = primary_exp->num;
        ident = primary_exp->ident;
    } else {
        assert(gst.isExist_func(ident));
        bool flag = gst.func_table[ident].second;
        vector<string>& params = gst.getParams(ident);
        vector<pair<string, pair<bool, int>>> args;
        if (func_rparams != nullptr) {
            reinterpret_cast<FuncRparamsAst&>(*func_rparams).dump(out, args);
        }
        for (int i = 0; i < args.size(); i++) {
            assert(args[i].first == params[i]);
        }
        if (flag) {
            out << "  %" << BaseAst::id << " = call @" << ident << "(";
            idx = BaseAst::id;
            BaseAst::id++;
        } else {
            out << "  call @" << ident << "(";
        }
        for (int i = 0; i < args.size(); i++) {
            if (args[i].second.first) {
                out << "%" << args[i].second.second;
            } else {
                out << args[i].second.second;
            }
            if (i != args.size() - 1) {
                out << ", ";
            }
        }
        out << ")" << std::endl;
    }
}

void FuncRparamsAst::dump(std::stringstream& out, vector<pair<string, pair<bool, int>>>& params) {
    auto ptr = reinterpret_cast<FuncRparamAst*>(func_rparam.get());
    ptr->dump(out);
    bool flag = (ptr->idx != -1);
    int num = (flag? ptr->idx : ptr->num);
    params.push_back(std::make_pair(ptr->btype, std::make_pair(flag, num)));
    if (func_rparams != nullptr) {
        reinterpret_cast<FuncRparamsAst&>(*func_rparams).dump(out, params);
    }
}

void FuncRparamAst::dump(std::stringstream& out) {
    exp->dump(out);
    idx = exp->idx;
    num = exp->num;
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
        count.cnt++;
        string then_label = count.getlabel("then");
        string end_label = count.getlabel("end");
        idx = BaseAst::id;
        string ident = "tmp_" + std::to_string(count.cnt);
        table.insert(ident, idx);
        printer.print_alloc(ident, out, table);
        printer.print_store(false, 0, ident, out, table);
        if (land_exp->idx == -1) {
            printer.print_br(false, land_exp->num, then_label, end_label, out);
        } else {
            printer.print_br(true, land_exp->idx, then_label, end_label, out);
        }
        printer.print_label(then_label, out);
        eq_exp->dump(out);
        idx = BaseAst::id;
        printer.print_eq(false, idx, eq_exp, out);
        printer.print_store(true, idx, ident, out, table);
        printer.print_jump(end_label, out);
        printer.print_label(end_label, out);
        printer.print_load(idx + 1, ident, out, table);
        BaseAst::id += 2;
        idx += 1;
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
        count.cnt++;
        string then_label = count.getlabel("then");
        string end_label = count.getlabel("end");
        idx = BaseAst::id;
        string ident = "tmp_" + std::to_string(count.cnt);
        table.insert(ident, idx);
        printer.print_alloc(ident, out, table);
        printer.print_store(false, 1, ident, out, table);
        if (lor_exp->idx == -1) {
            printer.print_br(false, lor_exp->num, end_label, then_label, out);
        } else {
            printer.print_br(true, lor_exp->idx, end_label, then_label, out);
        }
        printer.print_label(then_label, out);
        land_exp->dump(out);
        idx = BaseAst::id;
        printer.print_eq(false, idx, land_exp, out);
        printer.print_store(true, idx, ident, out, table);
        printer.print_jump(end_label, out);
        printer.print_label(end_label, out);
        printer.print_load(idx + 1, ident, out, table);
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
        const_decl->idx = idx;
        const_decl->cal();
    } else {
        var_decl->idx = idx;
        var_decl->dump(out);
    }
}

int ConstDeclAst::cal() {
    assert(const_def_list != nullptr);
    const_def_list->idx = idx;
    const_def_list->cal();
    return 0;
}

void VarDeclAst::dump(std::stringstream& out) {
    var_def_list->idx = idx;
    var_def_list->dump(out);
}

int ConstDefListAst::cal() {
    assert(const_def != nullptr);
    const_def->idx = idx;
    const_def->cal();
    if (const_def_list != nullptr) {
        const_def_list->idx = idx;
        const_def_list->cal();
    }
    return 0;
}

void VarDefListAst::dump(std::stringstream& out) {
    var_def->idx = idx;
    var_def->dump(out);
    if (var_def_list != nullptr) {
        var_def_list->idx = idx;
        var_def_list->dump(out);
    }
}

int ConstDefAst::cal() {
    assert(const_init_val != nullptr);
    int num = const_init_val->cal();
    if (idx != -1) {
        table.insert(ident, num);
    } else {
        table.gst.insert_var(ident, num, true);
    }
    table.insert(ident, num);
    return 0;
}

void VarDefAst::dump(std::stringstream& out) {
    table.insert(ident);
    if (idx != -1) {
        printer.print_alloc(ident, out, table);
        if (initval != nullptr) {
            initval->dump(out);
            if (initval->idx == -1) {
                printer.print_store(false, initval->num, ident, out, table);
            } else {
                printer.print_store(true, initval->idx, ident, out, table);
            }
        }
    } else {
        if (initval != nullptr) {
            table.gst.insert_var(ident, 0, false);
            printer.print_global_alloc(ident, initval->cal(), out, table.gst);
        } else {
            table.gst.insert_var(ident, 0, false);
            printer.print_global_alloc(ident, 0, out, table.gst);
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

int InitValAst::cal() {
    return exp->cal();
}

