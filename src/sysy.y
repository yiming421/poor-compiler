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
%token <str_val> IDENT MUL_OP ADD_OP CMP_OP EQ_OP LAND_OP LOR_OP OTHER_OP
%token <int_val> INT_CONST

%type <ast> FuncDef FuncType Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp
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

Stmt: RETURN Exp ';' {
    auto ast = new StmtAst();
    ast->exp = unique_ptr<BaseAst>($2);
    $$ = ast;
};

Exp: LOrExp {
    auto ast = new ExpAst();
    ast->lor_exp = unique_ptr<BaseAst>($1);
    $$ = ast;
};

PrimaryExp: '(' Exp ')' {
    auto ast = new PrimaryExpAst();
    ast->exp = unique_ptr<BaseAst>($2);
    ast->type = 0;
    $$ = ast;
} | Number {
    auto ast = new PrimaryExpAst();
    ast->number = $1;
    ast->type = 1;
    $$ = ast;
};

UnaryExp: PrimaryExp {
    auto ast = new UnaryExpAst();
    ast->primary_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | ADD_OP UnaryExp {
    auto ast = new UnaryExpAst();
    ast->op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAst>($2);
    ast->type = 1;
    $$ = ast;
} | OTHER_OP UnaryExp {
    auto ast = new UnaryExpAst();
    ast->op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAst>($2);
    ast->type = 1;
    $$ = ast;
};

AddExp: MulExp {
    auto ast = new AddExpAst();
    ast->mul_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | AddExp ADD_OP MulExp {
    auto ast = new AddExpAst();
    ast->add_exp = unique_ptr<BaseAst>($1);
    ast->op = *unique_ptr<string>($2);
    ast->mul_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

MulExp: UnaryExp {
    auto ast = new MulExpAst();
    ast->unary_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | MulExp MUL_OP UnaryExp {
    auto ast = new MulExpAst();
    ast->mul_exp = unique_ptr<BaseAst>($1);
    ast->op = *unique_ptr<string>($2);
    ast->unary_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

RelExp: AddExp {
    auto ast = new RelExpAst();
    ast->add_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | RelExp CMP_OP AddExp {
    auto ast = new RelExpAst();
    ast->rel_exp = unique_ptr<BaseAst>($1);
    ast->op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

EqExp: RelExp {
    auto ast = new EqExpAst();
    ast->rel_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | EqExp EQ_OP RelExp {
    auto ast = new EqExpAst();
    ast->eq_exp = unique_ptr<BaseAst>($1);
    ast->op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

LAndExp: EqExp {
    auto ast = new LAndExpAst();
    ast->eq_exp = unique_ptr<BaseAst>($1);
    ast->type = 0;
    $$ = ast;
} | LAndExp LAND_OP EqExp {
    auto ast = new LAndExpAst();
    ast->land_exp = unique_ptr<BaseAst>($1);
    ast->eq_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

LOrExp: LAndExp {
    auto ast = new LOrExpAst();
    ast->land_exp = unique_ptr<BaseAst>($1);
    $$ = ast;
    ast->type = 0;
} | LOrExp LOR_OP LAndExp {
    auto ast = new LOrExpAst();
    ast->lor_exp = unique_ptr<BaseAst>($1);
    ast->land_exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
};

Number: INT_CONST {
    $$ = $1;
};

%%

void yyerror(std::unique_ptr<BaseAst>& ast, const char* s) {
    cerr << "error: " << s << endl;
}

