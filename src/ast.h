#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>

using std::vector;
using std::string;

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
    std::unique_ptr<BaseAst> func_def;
    void dump(std::stringstream& out);
};

class FuncDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> func_type;
    std::string ident;
    std::unique_ptr<BaseAst> block;
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
    std::unique_ptr<BaseAst> exp;
    std::unique_ptr<BaseAst> lval;
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
    int type = 0;
    void dump(std::stringstream& out);
    int cal();
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
    int cal();
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
    void dump(std::stringstream& out);
};

class BtypeAst : public BaseAst {
public:
    string type = "int";
};

class ConstDefListAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_def;
    std::unique_ptr<BaseAst> const_def_list;
    int cal();
};

class ConstDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_init_val;
    int cal();
};

class ConstInitValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_exp;
    int cal();
};

class InitValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    void dump(std::stringstream& out);
};

class ConstExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    int cal();
};

class LValAst : public BaseAst {
public:
    int cal();
};
