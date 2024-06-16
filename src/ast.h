#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>

using std::vector;
using std::string;
using std::pair;

class BaseAst {
protected:
    static int id;
public:
    int idx = 0;
    int num = 0;
    string ident;
    virtual ~BaseAst() = default;
    virtual void dump(std::stringstream& out) {};
    virtual int cal() { return 0; };
};

class CompUnitAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> comp_unit_list;
    void dump(std::stringstream& out);
};

class CompUnitListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> decl_or_def;
    std::unique_ptr<BaseAst> comp_unit_list;
    void dump(std::stringstream& out);
};

class DeclOrDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> decl;
    std::unique_ptr<BaseAst> func_def;
    void dump(std::stringstream& out);
};


class FuncDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> func_type;
    std::unique_ptr<BaseAst> func_fparams;
    std::unique_ptr<BaseAst> block;
    void dump(std::stringstream& out);
};

class FuncFparamsAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> func_fparams;
    std::unique_ptr<BaseAst> func_fparam;
    void dump(std::stringstream& out, vector<pair<string, string>>& params); 
    void dump(std::stringstream& out) {}
};

class FuncFparamAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> btype;
    std::string type = "i32";
    std::unique_ptr<BaseAst> exp_list;
    int type_flag = 0;
    void dump(std::stringstream& out);
};

class FuncTypeAst : public BaseAst {
public:
    std::string type = "i32";
    void dump(std::stringstream& out);
};

class BlockAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> blockitem_list;
    void dump(std::stringstream& out);
    void dump(std::stringstream& out, vector<pair<string, string>>& params);
    bool flag = false;
};

class BlockItemListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> blockitem;
    std::unique_ptr<BaseAst> blockitem_list;
    void dump(std::stringstream& out);
};

class BlockItemAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> stmt = nullptr;
    std::unique_ptr<BaseAst> decl = nullptr;
    void dump(std::stringstream& out);
};

class StmtAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> if_stmt;
    std::unique_ptr<BaseAst> with_else;
    void dump(std::stringstream& out);
};

class WithElseAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> if_withelse;
    std::unique_ptr<BaseAst> else_withelse;
    std::unique_ptr<BaseAst> other_stmt;
    void dump(std::stringstream& out);
};

class IfStmtAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> stmt;
    std::unique_ptr<BaseAst> with_else;
    std::unique_ptr<BaseAst> if_stmt;
    void dump(std::stringstream& out);
};

class OtherStmtAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> lval;
    std::unique_ptr<BaseAst> block;
    std::unique_ptr<BaseAst> stmt;
    int type = 0;
    void dump(std::stringstream& out);
};

class ExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> lor_exp;
    void dump(std::stringstream& out);
    int cal();
};

class PrimaryExpAst : public BaseAst {
public:
    int number;
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> lval;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class UnaryExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> primary_exp;
    string op;
    std::unique_ptr<BaseAst> unary_exp;
    string ident;
    std::unique_ptr<BaseAst> func_rparams;

    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class FuncRparamsAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> func_rparams;
    std::unique_ptr<BaseAst> func_rparam;
    void dump(std::stringstream& out) {}
    void dump(std::stringstream& out, vector<pair<string, pair<bool, int>>>& params);
};

class FuncRparamAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::string btype = "i32";
    void dump(std::stringstream& out);
};

class AddExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> add_exp;
    std::unique_ptr<BaseAst> mul_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class MulExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> mul_exp;
    std::unique_ptr<BaseAst> unary_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class RelExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> rel_exp;
    std::unique_ptr<BaseAst> add_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class EqExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> eq_exp;
    std::unique_ptr<BaseAst> rel_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class LAndExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> land_exp;
    std::unique_ptr<BaseAst> eq_exp;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class LOrExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> lor_exp;
    std::unique_ptr<BaseAst> land_exp;
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
};

class DeclAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_decl;
    std::unique_ptr<BaseAst> var_decl;
    void dump(std::stringstream& out);
};

class ConstDeclAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> btype;
    std::unique_ptr<BaseAst> const_def_list;
    void dump(std::stringstream& out);
};

class VarDeclAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> btype;
    std::unique_ptr<BaseAst> var_def_list;
    void dump(std::stringstream& out);
};

class VarDefListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> var_def;
    std::unique_ptr<BaseAst> var_def_list;
    void dump(std::stringstream& out);
};

class VarDefAst : public BaseAst {
public:
    string ident;
    std::unique_ptr<BaseAst> initval;
    std::unique_ptr<BaseAst> const_exp_list;
    void dump(std::stringstream& out);
};

class BTypeAst : public BaseAst {
public:
    void dump(std::stringstream& out);
};

class ConstDefListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_def;
    std::unique_ptr<BaseAst> const_def_list;
    void dump(std::stringstream& out);
};

class ConstDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_init_val;
    std::unique_ptr<BaseAst> const_exp_list;
    int cal();
    void dump(std::stringstream& out);
};

class ConstExpListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_exp;
    std::unique_ptr<BaseAst> const_exp_list;
    int cal() {return 0;}
    void cal(vector<int>& nums);
};

class ConstInitValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_exp;
    std::unique_ptr<BaseAst> const_init_val_list;
    int cal();
    void dump(std::stringstream& out, string& ident, vector<int>& nums, bool flag); // print init value
    void cal(vector<int>& nums, int idx, int& cnt, vector<int>& data); // helper function
    void dump(std::stringstream& out) {}
};

class InitValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> init_val_list;
    void dump(std::stringstream& out);
    void dump(std::stringstream& out, string& ident, vector<int>& num, bool flag);
    void dump(vector<int>& nums, vector<pair<int, bool>>& data, int idx, int& cnt, std::stringstream& out); // helper function
    int cal();
};

class ConstExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    int cal();
};

class LValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp_list;
    int cal();
};

class ExpListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> exp_list;
    int cal() {return 0;}
    void cal(vector<int>& nums);
    void dump(vector<pair<int, bool>>& nums, std::stringstream& out);
    void dump(std::stringstream& out) {}
};

class InitValListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> init_val;
    std::unique_ptr<BaseAst> init_val_list;
    void dump(std::stringstream& out) {}
    void dump(vector<int>& nums, vector<pair<int, bool>>& data, int idx, int& cnt, std::stringstream& out, bool flag); // helper function
};

class ConstInitValListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_init_val;
    std::unique_ptr<BaseAst> const_init_val_list;
    void cal(vector<int>& nums, int idx, int& cnt, vector<int>& data, bool flag); // helper function
    int cal() {return 0;}
};
