#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
using std::string;

class BaseAst {
protected:
    static int id;
public:
    int idx = 0;
    int num = 0;
    virtual ~BaseAst() = default;
    virtual void dump(std::stringstream& out) = 0;
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
    std::unique_ptr<BaseAst> stmt;
    void dump(std::stringstream& out);
};

class StmtAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> exp;
    void dump(std::stringstream& out);
};

class ExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> lor_exp;
    void dump(std::stringstream& out);
};

class PrimaryExpAst : public BaseAst {
public:
    int number;
    std::unique_ptr<BaseAst> exp;
    int type = 0;
    void dump(std::stringstream& out);
};

class UnaryExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> primary_exp;
    string op;
    std::unique_ptr<BaseAst> unary_exp;
    int type = 0;
    void dump(std::stringstream& out);
};

class AddExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> add_exp;
    std::unique_ptr<BaseAst> mul_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
};

class MulExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> mul_exp;
    std::unique_ptr<BaseAst> unary_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
};

class RelExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> rel_exp;
    std::unique_ptr<BaseAst> add_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
};

class EqExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> eq_exp;
    std::unique_ptr<BaseAst> rel_exp;
    string op;
    int type = 0;
    void dump(std::stringstream& out);
};

class LAndExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> land_exp;
    std::unique_ptr<BaseAst> eq_exp;
    int type = 0;
    void dump(std::stringstream& out);
};

class LOrExpAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> lor_exp;
    std::unique_ptr<BaseAst> land_exp;
    int type = 0;
    void dump(std::stringstream& out);
};