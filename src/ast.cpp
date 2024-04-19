#include "ast.h"
#include <sstream>

int BaseAst::id = 0;

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
    stmt->dump(out);
    out << "}" << std::endl;
}

void StmtAst::dump(std::stringstream& out) {
    exp->dump(out);
    if (exp->idx == -1) {
        out << "  ret " << exp->num << std::endl;
    } else {
        out << "  ret %" << exp->idx << std::endl;
    }
}

void ExpAst::dump(std::stringstream& out) {
    lor_exp->dump(out);
    idx = lor_exp->idx;
    num = lor_exp->num;
}

void PrimaryExpAst::dump(std::stringstream& out) {
    if (type == 1){
        idx = -1;
        num = number;
    } else {
        exp->dump(out);
        idx = exp->idx;
        num = exp->num;
    }
}

void UnaryExpAst::dump(std::stringstream& out) {
    if (type != 0){
        unary_exp->dump(out);
        idx = BaseAst::id;
        if (op == "-"){
            if (unary_exp->idx != -1) {
                out << "  %" << idx << " = sub 0, %" << unary_exp->idx << std::endl;
            } else {
                out << "  %" << idx << " = sub 0, " << unary_exp->num << std::endl;
            }
            BaseAst::id++;
        } else if (op == "!"){
            if (unary_exp->idx != -1) {
                out << "  %" << idx << " = eq 0, %" << unary_exp->idx << std::endl;
            } else {
                out << "  %" << idx << " = eq 0, " << unary_exp->num << std::endl;
            }
            BaseAst::id++;
        } else {
            idx = unary_exp->idx;
            num = unary_exp->num;
        }
    } else {
        primary_exp->dump(out);
        idx = primary_exp->idx;
        num = primary_exp->num;
    }
}

void AddExpAst::dump(std::stringstream& out) {
    if (type != 0){
        add_exp->dump(out);
        mul_exp->dump(out);
        idx = BaseAst::id;
        if (op == "+"){
            out << "  %" << idx << " = add ";
        } else {
            out << "  %" << idx << " = sub ";
        }
        if (add_exp->idx != -1) {
            out << "%" << add_exp->idx << ", ";
        } else {
            out << add_exp->num << ", ";
        }
        if (mul_exp->idx != -1) {
            out << "%" << mul_exp->idx << std::endl;
        } else {
            out << mul_exp->num << std::endl;
        }
        BaseAst::id++;
    } else {
        mul_exp->dump(out);
        idx = mul_exp->idx;
        num = mul_exp->num;
    }
}

void MulExpAst::dump(std::stringstream& out) {
    if (type != 0){
        mul_exp->dump(out);
        unary_exp->dump(out);
        idx = BaseAst::id;
        if (op == "*"){
            out << "  %" << idx << " = mul ";
        } else if (op == "/"){
            out << "  %" << idx << " = div ";
        } else {
            out << "  %" << idx << " = mod ";
        }
        if (mul_exp->idx != -1) {
            out << "%" << mul_exp->idx << ", ";
        } else {
            out << mul_exp->num << ", ";
        }
        if (unary_exp->idx != -1) {
            out << "%" << unary_exp->idx << std::endl;
        } else {
            out << unary_exp->num << std::endl;
        }
        BaseAst::id++;
    } else {
        unary_exp->dump(out);
        idx = unary_exp->idx;
        num = unary_exp->num;
    }
}

void RelExpAst::dump(std::stringstream& out) {
    if (type != 0){
        rel_exp->dump(out);
        add_exp->dump(out);
        idx = BaseAst::id;
        if (op == "<"){
            out << "  %" << idx << " = lt ";
        } else if (op == "<="){
            out << "  %" << idx << " = le ";
        } else if (op == ">"){
            out << "  %" << idx << " = gt ";
        } else {
            out << "  %" << idx << " = ge ";
        }
        if (rel_exp->idx != -1) {
            out << "%" << rel_exp->idx << ", ";
        } else {
            out << rel_exp->num << ", ";
        }
        if (add_exp->idx != -1) {
            out << "%" << add_exp->idx << std::endl;
        } else {
            out << add_exp->num << std::endl;
        }
        BaseAst::id++;
    } else {
        add_exp->dump(out);
        idx = add_exp->idx;
        num = add_exp->num;
    }
}

void EqExpAst::dump(std::stringstream& out) {
    if (type != 0){
        eq_exp->dump(out);
        rel_exp->dump(out);
        idx = BaseAst::id;
        if (op == "=="){
            out << "  %" << idx << " = eq ";
        } else {
            out << "  %" << idx << " = ne ";
        }
        if (eq_exp->idx != -1) {
            out << "%" << eq_exp->idx << ", ";
        } else {
            out << eq_exp->num << ", ";
        }
        if (rel_exp->idx != -1) {
            out << "%" << rel_exp->idx << std::endl;
        } else {
            out << rel_exp->num << std::endl;
        }
        BaseAst::id++;
    } else {
        rel_exp->dump(out);
        idx = rel_exp->idx;
        num = rel_exp->num;
    }
}

void LAndExpAst::dump(std::stringstream& out) {
    if (type != 0){
        land_exp->dump(out);
        eq_exp->dump(out);
        idx = BaseAst::id;
        out << "  %" << idx << " = ne 0, ";
        if (land_exp->idx != -1) {
            out << "%" << land_exp->idx << std::endl;
        } else {
            out << land_exp->num << std::endl;
        }
        out << "  %" << idx + 1 << " = ne 0, "; 
        if (eq_exp->idx != -1) {
            out << "%" << eq_exp->idx << std::endl;
        } else {
            out << eq_exp->num << std::endl;
        }
        out << "  %" << idx + 2 << " = and %" << idx << ", %" << idx + 1 << std::endl;
        BaseAst::id += 3;
        idx += 2;
    } else {
        eq_exp->dump(out);
        idx = eq_exp->idx;
        num = eq_exp->num;
    }
}

void LOrExpAst::dump(std::stringstream& out) {
    if (type != 0){
        lor_exp->dump(out);
        land_exp->dump(out);
        idx = BaseAst::id;
        out << "  %" << idx << " = or ";
        if (lor_exp->idx != -1) {
            out << "%" << lor_exp->idx << ", ";
        } else {
            out << lor_exp->num << ", ";
        }
        if (land_exp->idx != -1) {
            out << "%" << land_exp->idx << std::endl;
        } else {
            out << land_exp->num << std::endl;
        }
        out << "  %" << idx + 1 << " = ne 0, %" << idx << std::endl;
        BaseAst::id += 2;
        idx += 1;
    } else {
        land_exp->dump(out);
        idx = land_exp->idx;
        num = land_exp->num;
    }
}