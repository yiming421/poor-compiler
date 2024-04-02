%code requires {
    #include <memory>
    #include <string>
    #include "ast.h"
}

%{


#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

int yylex();
void yyerror(std::unique_ptr<BaseAst>& ast, const char* s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAst>& ast }

%union {
    std::string* str_val;
    int int_val;
    BaseAst* ast;
}

%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast> FuncDef FuncType Block Stmt
%type <int_val> Number

%%

CompUnit: FuncDef {
    auto comp_unit = make_unique<CompUnitAst>();
    comp_unit->func_def = unique_ptr<BaseAst>($1);
    ast = move(comp_unit);
};

FuncDef: FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAst();
    ast->func_type = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAst>($5);
    $$ = ast;
};

FuncType: INT {
    auto ast = new FuncTypeAst();
    $$ = ast;
};

Block: '{' Stmt '}' {
    auto ast = new BlockAst();
    ast->stmt = unique_ptr<BaseAst>($2);
    $$ = ast;
};

Stmt: RETURN Number ';' {
    auto ast = new StmtAst();
    ast->number = $2;
    $$ = ast;
};

Number: INT_CONST {
    $$ = $1;
};

%%

void yyerror(std::unique_ptr<BaseAst>& ast, const char* s) {
    cerr << "error: " << s << endl;
}

