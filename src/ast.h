#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

class BaseAst {
public:
    virtual ~BaseAst() = default;
    virtual void dump(std::stringstream& out) const = 0;
};

class CompUnitAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> func_def;
    void dump(std::stringstream& out) const override{
        func_def->dump(out);
    }
};

class FuncDefAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> func_type;
    std::string ident;
    std::unique_ptr<BaseAst> block;
    void dump(std::stringstream& out) const override{
        out << "fun @" << ident << "(): ";
        func_type->dump(out);
        block->dump(out);
    }
};

class FuncTypeAst : public BaseAst {
public:
    std::string type = "i32";
    void dump(std::stringstream& out) const override{
        out << type << " ";
    }
};

class BlockAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> stmt;
    void dump(std::stringstream& out) const override{
        out << "{" << std::endl;
        out << "@entry:" << std::endl;
        stmt->dump(out);
        out << "}" << std::endl;
    }
};

class StmtAst : public BaseAst {
public:
    int number;
    void dump(std::stringstream& out) const override{
        out << "  " << "ret " << number << std::endl;
    }
};
