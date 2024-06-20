#include "ast.h"
#include "util.h"
#include "symbol.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <cassert>

int BaseAst::id = 0;

Printer printer; // Printer for printing IR
SymbolTable table; // Symbol table for storing variables and functions
GlobalSymbolTable& gst = table.gst; // Global symbol table
bool end = false; // Flag for ending a function
bs_cnt count; // Counter for generating labels
bool flag_main = false; // Flag for main function
bool flag_type = false; // Flag for return type of a function
bool flag_global_init = false; // Flag for global initialization
 
void CompUnitAst::dump(std::stringstream& out) { 
    printer.print_decl(out);
    comp_unit_list->dump(out);
    assert(flag_main); // Main function must be defined
}

void CompUnitListAst::dump(std::stringstream& out) {
    decl_or_def->dump(out);
    if (comp_unit_list != nullptr) {
        comp_unit_list->dump(out);
    }
}

void DeclOrDefAst::dump(std::stringstream& out) {
    if (decl != nullptr) {
        decl->idx = -1; // Global declaration
        decl->dump(out);
    } else {
        func_def->dump(out);
    }
}

void FuncDefAst::dump(std::stringstream& out) {
    table.clear();
    reinterpret_cast<BlockAst&>(*block).flag = true; // Flag for entry block
    out << "fun @" << ident << "(";
    if (ident == "main") {
        flag_main = true;
    }
    vector<pair<string, string>> params;
    if (func_fparams != nullptr) {
        reinterpret_cast<FuncFparamsAst&>(*func_fparams).dump(out, params);
    }
    bool flag = false;
    if (func_type->ident == "i32") {
        flag = true;
    } else {
        flag = false;
    }
    assert(gst.insert_func(ident, flag));
    out << ")";
    func_type->dump(out);
    out << " {" << std::endl;
    end = false;
    reinterpret_cast<BlockAst&>(*block).dump(out, params);
    if (!end) { // If no return statement, add a return statement
        if (flag_type) {
            out << "  ret 0" << std::endl;
        } else {
            out << "  ret" << std::endl;
        } 
    }
    out << "}" << std::endl;
    end = false;
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
    if (exp_list == nullptr) {
        if (type_flag == 1) {
            table.insert(ident, true, 1);
            printer.print_funcfparam(ident, out, table, true);
            type = "*" + type; // Pointer type
        } else {
            table.insert(ident, false, 0);
            printer.print_funcfparam(ident, out, table, false);
        }
    } else {
        vector<int> nums;
        reinterpret_cast<ExpListAst&>(*exp_list).cal(nums);
        table.insert(ident, true, nums.size() + 1);
        printer.print_funcfparam(ident, nums, out, table); 
        std::stringstream ss;
        printer.recursive_print(nums, 0, ss); // Print array type
        type = ss.str();
        type = "*" + type;
    }
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

void BlockAst::dump(std::stringstream& out, vector<pair<string, string>>& params) { // Function block
    out << "%entry:" << std::endl;
    for (auto& param : params) {
        int num = table.getID(param.second);
        out << "  @" << param.second + '_' + std::to_string(num) << " = alloc " << param.first << std::endl;
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

void IfStmtAst::dump(std::stringstream& out) { // If stmt, without else
    if (stmt != nullptr) {
        end = false;
        exp->dump(out);
        count.cnt++;
        string then_label = count.getlabel("then");
        string end_label = count.getlabel("end");
        if (exp->idx == -1) { // Constant expression
            printer.print_br(false, exp->num, then_label, end_label, out);
        } else { // Variable expression
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
        end = false; // Flag for ending a function
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
        if (!end) { // only print jump when there is no return statement
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

void WithElseAst::dump(std::stringstream& out) { // If stmt, with else, solving dangling else
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
        case 0: // ret stmt
            exp->dump(out);
            if (exp->idx == -1) {
                out << "  ret " << exp->num << std::endl;
            } else {
                out << "  ret %" << exp->idx << std::endl;
            }
            end = true; // Flag for ending a function
            break;
        case 1: // assignment stmt
            if (reinterpret_cast<LValAst&>(*lval).exp_list == nullptr) { // Variable
                exp->dump(out);
                if (exp->idx == -1) {
                    printer.print_store(false, exp->num, lval->ident, out, table);
                } else {
                    printer.print_store(true, exp->idx, lval->ident, out, table);
                }
                
            } else { // Array
                exp->dump(out);
                vector<pair<int, bool>> nums;
                auto& exp_list = reinterpret_cast<LValAst&>(*lval).exp_list;
                reinterpret_cast<ExpListAst&>(*exp_list).dump(nums, out);
                int num = table.get(lval->ident);
                bool is_ptr = (num > 0);
                if (exp->idx == -1) {
                    printer.print_store_array_const(lval->ident, nums, BaseAst::id, exp->num, out, table, is_ptr);
                    BaseAst::id += nums.size() + 1;
                } else {
                    printer.print_store_array_var(lval->ident, nums, BaseAst::id, exp->idx, out, table, is_ptr);
                    BaseAst::id += nums.size() + 2;
                }
            }
            break;
        case 2: // block stmt
            table.push();
            block->idx = idx;
            block->dump(out);
            table.pop();
            break;
        case 3: // exp stmt
            exp->dump(out);
            break;
        case 5: { // while stmt
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
        case 7: { // break stmt
            assert(idx != 0);
            string end_label = count.getlabel("end", idx);
            printer.print_jump(end_label, out);
            end = true;
            break;
        }
        case 6: { // continue stmt
            assert(idx != 0);
            string entry_label = count.getlabel("entry", idx);
            printer.print_jump(entry_label, out);
            end = true;
            break;
        }
        case 8:  // return stmt
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
    idx = lor_exp->idx; // index for register
    num = lor_exp->num; // number for constant
}

int ExpAst::cal() {
    return lor_exp->cal();
}

void PrimaryExpAst::dump(std::stringstream& out) {
    if (type == 1){ // Constant
        idx = -1;
        num = number;
    } else if (type == 0){ // Expression
        exp->dump(out);
        idx = exp->idx;
        num = exp->num;
    } else { // LVal
        if (table.isConst(lval->ident)) {
            idx = -1;
            num = lval->cal(); // Constant
        } else {
            if (reinterpret_cast<LValAst&>(*lval).exp_list == nullptr) { // Variable
                idx = BaseAst::id;
                printer.print_load(idx, lval->ident, out, table);
                BaseAst::id++;
            } else { // Array
                vector<pair<int, bool>> nums;
                auto& exp_list = reinterpret_cast<LValAst&>(*lval).exp_list;
                reinterpret_cast<ExpListAst&>(*exp_list).dump(nums, out);
                bool is_ptr = (table.get(lval->ident) > 0);
                printer.print_load_array(lval->ident, nums, BaseAst::id, out, table, is_ptr);
                if (is_ptr) {
                    BaseAst::id += nums.size() + 2;
                } else {
                    BaseAst::id += nums.size() + 1;
                }
                idx = BaseAst::id - 1;
            }
        }
    }
}

int PrimaryExpAst::cal() { // Calculate the value of primary expression
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
        if (unary_exp->idx == -1) {
            idx = -1;
            if (op == "-"){
                num = -unary_exp->num;
            } else if (op == "+") {
                num = unary_exp->num;
            } else {
                num = !unary_exp->num;
            }
        } else {
            idx = BaseAst::id;
            if (op != "+"){
                printer.print_unary(idx, op, unary_exp, out);
                BaseAst::id++;
            } else {
                idx = unary_exp->idx;
                num = unary_exp->num;
            }
        }
    } else if (type == 0) {
        primary_exp->dump(out);
        idx = primary_exp->idx;
        num = primary_exp->num;
    } else {
        assert(gst.isExist_func(ident));
        bool flag = gst.func_table[ident];
        vector<pair<string, pair<bool, int>>> args;
        if (func_rparams != nullptr) {
            reinterpret_cast<FuncRparamsAst&>(*func_rparams).dump(out, args); // Get parameters
        }
        if (flag) {
            out << "  %" << BaseAst::id << " = call @" << ident << "(";
            idx = BaseAst::id;
            BaseAst::id++;
        } else {
            out << "  call @" << ident << "(";
        }
        for (int i = 0; i < args.size(); i++) { // Print parameters
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
    int num = (flag? ptr->idx : ptr->num); // Get the value of parameter
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
        if (add_exp->idx == -1 && mul_exp->idx == -1) {
            idx = -1;
            if (op == "+"){
                num = add_exp->num + mul_exp->num;
            } else {
                num = add_exp->num - mul_exp->num;
            }
        } else {
            idx = BaseAst::id;
            printer.print_binary(idx, op, add_exp, mul_exp, out);
            BaseAst::id++;
        }
    } else {
        mul_exp->dump(out);
        idx = mul_exp->idx;
        num = mul_exp->num;
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
        if (mul_exp->idx == -1 && unary_exp->idx == -1) {
            idx = -1;
            if (op == "*"){
                num = mul_exp->num * unary_exp->num;
            } else if (op == "/"){
                num = mul_exp->num / unary_exp->num;
            } else {
                num = mul_exp->num % unary_exp->num;
            }
        } else {
            idx = BaseAst::id;
            printer.print_binary(idx, op, mul_exp, unary_exp, out);
            BaseAst::id++;
        }
    } else {
        unary_exp->dump(out);
        idx = unary_exp->idx;
        num = unary_exp->num;
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
        if (rel_exp->idx == -1 && add_exp->idx == -1) {
            idx = -1;
            if (op == "<"){
                num = rel_exp->num < add_exp->num;
            } else if (op == "<="){
                num = rel_exp->num <= add_exp->num;
            } else if (op == ">"){
                num = rel_exp->num > add_exp->num;
            } else {
                num = rel_exp->num >= add_exp->num;
            }
        } else {
            idx = BaseAst::id;
            printer.print_binary(idx, op, rel_exp, add_exp, out);
            BaseAst::id++;
        }
    } else {
        add_exp->dump(out);
        idx = add_exp->idx;
        num = add_exp->num;
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
        if (eq_exp->idx == -1 && rel_exp->idx == -1) {
            idx = -1;
            if (op == "=="){
                num = eq_exp->num == rel_exp->num;
            } else {
                num = eq_exp->num != rel_exp->num;
            }
        } else {
            idx = BaseAst::id;
            printer.print_binary(idx, op, eq_exp, rel_exp, out);
            BaseAst::id++;
        }
    } else {
        rel_exp->dump(out);
        idx = rel_exp->idx;
        num = rel_exp->num;
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
        string then_label = count.getlabel("then"); // implement short-circuit evaluation
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
        string then_label = count.getlabel("then"); // implement short-circuit evaluation
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
    if (end) {
        return;
    }
    if (const_decl != nullptr) {
        const_decl->idx = idx;
        const_decl->dump(out);
    } else {
        var_decl->idx = idx;
        var_decl->dump(out);
    }
}

void ConstDeclAst::dump(std::stringstream& out) {
    assert(const_def_list != nullptr);
    const_def_list->idx = idx;
    const_def_list->dump(out);
}

void VarDeclAst::dump(std::stringstream& out) {
    var_def_list->idx = idx;
    var_def_list->dump(out);
}

void ConstDefListAst::dump(std::stringstream& out) {
    assert(const_def != nullptr);
    const_def->idx = idx;
    if (reinterpret_cast<ConstDefAst&>(*const_def).const_exp_list == nullptr) {
        const_def->cal();
    } else {
        const_def->dump(out);
    }
    if (const_def_list != nullptr) {
        const_def_list->idx = idx;
        const_def_list->dump(out);
    }
}

void VarDefListAst::dump(std::stringstream& out) {
    var_def->idx = idx;
    var_def->dump(out);
    if (var_def_list != nullptr) {
        var_def_list->idx = idx;
        var_def_list->dump(out);
    }
}

int ConstDefAst::cal() { // when we call cal, the constdef must be a variable
    assert(const_exp_list == nullptr);
    assert(const_init_val != nullptr);
    int num = const_init_val->cal();
    if (idx != -1) {
        table.insert(ident, num);
    } else {
        table.gst.insert_var(ident, num, true);
    }
    return 0;
}

void ConstDefAst::dump(std::stringstream& out) { // when we call dump, the constdef must be an array
    vector<int> nums;
    assert(const_exp_list != nullptr);
    reinterpret_cast<ConstExpListAst&>(*const_exp_list).cal(nums);  
    if (idx != -1) {
        table.insert(ident, false, nums.size());
        printer.print_alloc_arr(ident, nums, out, table);
        reinterpret_cast<ConstInitValAst&>(*const_init_val).dump(out, ident, nums, true);
    } else {
        table.gst.insert_var(ident, nums.size(), false);
        printer.print_global_alloc_arr(ident, nums, out, table.gst);
        reinterpret_cast<ConstInitValAst&>(*const_init_val).dump(out, ident, nums, false);
    }
}

void VarDefAst::dump(std::stringstream& out) {
    if (idx != -1) { // Local variable
        if (const_exp_list == nullptr) { // Variable
            table.insert(ident, false, 0);
            printer.print_alloc(ident, out, table);
            if (initval != nullptr) {
                initval->dump(out);
                if (initval->idx == -1) {
                    printer.print_store(false, initval->num, ident, out, table);
                } else {
                    printer.print_store(true, initval->idx, ident, out, table);
                }
            }
        } else { // Array
            vector<int> nums;
            reinterpret_cast<ConstExpListAst&>(*const_exp_list).cal(nums); // Get the size of array
            table.insert(ident, false, nums.size());
            printer.print_alloc_arr(ident, nums, out, table);
            if (initval != nullptr) {
                reinterpret_cast<InitValAst&>(*initval).dump(out, ident, nums, true);
            } else {
                out << std::endl;
            }
        }
    } else { // Global variable
        if (const_exp_list == nullptr) { // Variable
            table.gst.insert_var(ident, 0, false);
            if (initval != nullptr) {
                printer.print_global_alloc(ident, initval->cal(), out, table.gst);
            } else {
                printer.print_global_alloc(ident, 0, out, table.gst);
            }
        } else { // Array
            vector<int> nums;
            reinterpret_cast<ConstExpListAst&>(*const_exp_list).cal(nums);
            table.gst.insert_var(ident, nums.size(), false);
            printer.print_global_alloc_arr(ident, nums, out, table.gst);
            if (initval != nullptr) {
                reinterpret_cast<InitValAst&>(*initval).dump(out, ident, nums, false);
            } else {
                out << ", zeroinit" << std::endl;
            }
        }
    }
}

void ConstExpListAst::cal(vector<int>& nums) {
    nums.push_back(const_exp->cal());
    if (const_exp_list != nullptr) {
        reinterpret_cast<ConstExpListAst&>(*const_exp_list).cal(nums);
    }
}

int ConstInitValAst::cal() {
    assert(const_exp != nullptr);
    return const_exp->cal();
}

void ConstInitValAst::dump(std::stringstream& out, string& ident, vector<int>& nums, bool flag) { 
    if (const_init_val_list == nullptr) {
        if (flag) { // Local variable
            out << std::endl;
        } else { // Global variable
            out << ", zeroinit" << std::endl;
        }
        return;
    }
    if (flag) {
        out << std::endl;
    }
    vector<int> data;
    vector<int> widths(nums.size()); // Calculate the width of each dimension
    widths[nums.size() - 1] = nums[nums.size() - 1];
    for (int i = nums.size() - 2; i >= 0; i--) {
        widths[i] = widths[i + 1] * nums[i];
    }
    int cnt = 0;
    reinterpret_cast<ConstInitValListAst&>(*const_init_val_list).cal(widths, 0, cnt, data, true);
    // fill data with array elements
    assert(cnt == widths[0]);
    if (flag) {
        printer.print_init_arr_const(ident, data, nums, BaseAst::id, out, table); // Print local array
    } else {
        printer.print_aggregate_const(data, nums, out); // Print global array
    }
}

void ConstInitValListAst::cal(vector<int>& nums, int idx, int& cnt, vector<int>& data, bool flag) { // helper function for dump
    int cnt_now = cnt;
    reinterpret_cast<ConstInitValAst&>(*const_init_val).cal(nums, abs(idx), cnt, data);
    if (const_init_val_list != nullptr) {
        reinterpret_cast<ConstInitValListAst&>(*const_init_val_list).cal(nums, idx, cnt, data, false);
    }
    if (flag) { // Fill the rest with 0
        for (int i = cnt - cnt_now; i < nums[idx]; i++) {
            data.push_back(0);
        }
    cnt = cnt_now + nums[idx]; // Update the count
    }
}

void ConstInitValAst::cal(vector<int>& nums, int idx, int& cnt, vector<int>& data) {
    if (const_exp != nullptr) {
        data.push_back(const_exp->cal());
        cnt++;
    } else {
        assert(cnt % nums[nums.size() - 1] == 0); // the count should be a multiple of the last dimension
        int i = nums.size() - 2;
        for (; i > idx; i--) { // find the first dimension that is not a multiple of the count
            if (cnt % nums[i] != 0) {
                break;
            }
        }
        if (const_init_val_list == nullptr) {
            for (int k = 0; k < nums[i + 1]; ++k) { // fill the rest with 0
                data.push_back(0);
                cnt++;
            }
            return;
        }
        reinterpret_cast<ConstInitValListAst&>(*const_init_val_list).cal(nums, i + 1, cnt, data, true);
    }
}

void InitValAst::dump(std::stringstream& out) {
    exp->dump(out);
    idx = exp->idx;
    num = exp->num;
}

void InitValAst::dump(std::stringstream& out, string& ident, vector<int>& nums, bool flag) {
    if (init_val_list == nullptr) {
        if (flag) { // Local variable
            out << std::endl;
        } else { // Global variable
            out << ", zeroinit" << std::endl;
        }
        return;
    }
    if (flag) { // Local variable
        out << std::endl;
    }
    vector<int> widths(nums.size());
    widths[nums.size() - 1] = nums[nums.size() - 1];
    for (int i = nums.size() - 2; i >= 0; i--) {
        widths[i] = widths[i + 1] * nums[i];
    }
    vector<pair<int, bool>> data;
    int cnt = 0;
    flag_global_init = !flag;
    reinterpret_cast<InitValListAst&>(*init_val_list).dump(widths, data, 0, cnt, out, true); 
    assert(cnt == widths[0]);
    idx = BaseAst::id;
    if (flag) {
        printer.print_init_arr_var(ident, data, nums, BaseAst::id, out, table);
    } else {
        printer.print_aggregate_var(data, nums, out);
    }
}

void InitValListAst::dump(vector<int>& nums, vector<pair<int, bool>>& data, int idx, int& cnt, std::stringstream& out, bool flag) {
    int cnt_now = cnt;
    reinterpret_cast<InitValAst&>(*init_val).dump(nums, data, idx, cnt, out);
    if (init_val_list != nullptr) {
        reinterpret_cast<InitValListAst&>(*init_val_list).dump(nums, data, idx, cnt, out, false);
    }
    if (flag) {
        for (int i = cnt - cnt_now; i < nums[idx]; i++) {
            data.push_back(std::make_pair(0, false));
        }
        cnt = cnt_now + nums[idx];
    }
}

void InitValAst::dump(vector<int>& nums, vector<pair<int, bool>>& data, int idx, int& cnt, std::stringstream& out) {
    if (exp != nullptr) {
        if (flag_global_init) {
            data.push_back(std::make_pair(exp->cal(), false));
        } else {
            exp->dump(out);
            if (exp->idx == -1) {
                data.push_back(std::make_pair(exp->num, false));
            } else {
                data.push_back(std::make_pair(exp->idx, true));
            }
        }
        cnt++;
    } else {
        assert(cnt % nums[nums.size() - 1] == 0);
        int i = nums.size() - 2;
        for (; i > idx; i--) {
            if (cnt % nums[i] != 0) {
                break;
            }
        }
        if (init_val_list == nullptr) {
            for (int k = 0; k < nums[i + 1]; ++k) {
                data.push_back(std::make_pair(0, false));
                cnt++;
            }
            return;
        }
        reinterpret_cast<InitValListAst&>(*init_val_list).dump(nums, data, i + 1, cnt, out, true);
    }
}

int ConstExpAst::cal() {
    return exp->cal();
}

int LValAst::cal() {
    assert(table.isExist(ident));
    if (table.isConst(ident)) {
        return table.get(ident);
    } else {
        return exp_list->cal();
    }
}

int InitValAst::cal() {
    return exp->cal();
}

void ExpListAst::cal(vector<int>& nums) {
    nums.push_back(exp->cal());
    if (exp_list != nullptr) {
        reinterpret_cast<ExpListAst&>(*exp_list).cal(nums);
    }
}

void ExpListAst::dump(vector<pair<int, bool>>& nums, std::stringstream& out) {
    exp->dump(out);
    if (exp->idx == -1) {
        nums.push_back(std::make_pair(exp->num, false));
    } else {
        nums.push_back(std::make_pair(exp->idx, true));
    }
    if (exp_list != nullptr) {
        reinterpret_cast<ExpListAst&>(*exp_list).dump(nums, out);
    }
}