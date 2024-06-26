%code requires {
    #include <memory>
    #include <string>
    #include <iostream>
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

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT MUL_OP ADD_OP CMP_OP EQ_OP LAND_OP LOR_OP OTHER_OP
%token <int_val> INT_CONST

%type <ast> FuncDef Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp 
Decl ConstDecl VarDecl VarDefList VarDef InitVal Btype ConstDefList ConstDef ConstInitVal BlockItem LVal 
ConstExp BlockItemList WithElse OtherStmt IfStmt CompUnitList FuncFParams FuncFParam FuncRParams FuncRParam 
DeclOrDef InitValList ConstInitValList ConstExpList ExpList
%type <int_val> Number

%%
CompUnit: CompUnitList {
    auto comp_unit = make_unique<CompUnitAst>();
    comp_unit->comp_unit_list = unique_ptr<BaseAst>($1);
    ast = move(comp_unit);
};

CompUnitList: DeclOrDef {
    auto ast = new CompUnitListAst();
    ast->decl_or_def = unique_ptr<BaseAst>($1);
    $$ = ast;
} | DeclOrDef CompUnitList{
    auto ast = new CompUnitListAst();
    ast->decl_or_def = unique_ptr<BaseAst>($1);
    ast->comp_unit_list = unique_ptr<BaseAst>($2);
    $$ = ast;
}

DeclOrDef: Decl {
    auto ast = new DeclOrDefAst();
    ast->decl = unique_ptr<BaseAst>($1);
    $$ = ast;
} | FuncDef {
    auto ast = new DeclOrDefAst();
    ast->func_def = unique_ptr<BaseAst>($1);
    $$ = ast;
};

FuncDef: Btype IDENT '(' ')' Block {
    auto ast = new FuncDefAst();
    ast->func_type = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAst>($5);
    $$ = ast;
} | Btype IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAst();
    ast->func_type = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_fparams = unique_ptr<BaseAst>($4);
    ast->block = unique_ptr<BaseAst>($6);
    $$ = ast;
};

FuncFParams: FuncFParam {
    auto ast = new FuncFparamsAst();
    ast->func_fparam = unique_ptr<BaseAst>($1);
    $$ = ast;
} | FuncFParam ',' FuncFParams {
    auto ast = new FuncFparamsAst();
    ast->func_fparam = unique_ptr<BaseAst>($1);
    ast->func_fparams = unique_ptr<BaseAst>($3);
    $$ = ast;
};

FuncFParam: Btype IDENT {
    auto ast = new FuncFparamAst();
    ast->btype = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->type_flag = 0;
    $$ = ast;
} | Btype IDENT '[' ']' ExpList {
    auto ast = new FuncFparamAst();
    ast->btype = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->exp_list = unique_ptr<BaseAst>($5);
    ast->type_flag = 1;
    $$ = ast;
} | Btype IDENT '[' ']' {
    auto ast = new FuncFparamAst();
    ast->btype = unique_ptr<BaseAst>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->type_flag = 1;
    $$ = ast;
};

Block: '{' BlockItemList '}' {
    auto ast = new BlockAst();
    ast->blockitem_list = unique_ptr<BaseAst>($2);
    $$ = ast;
} | '{' '}' {
    auto ast = new BlockAst();
    $$ = ast;
};

BlockItemList: BlockItem {
    auto ast = new BlockItemListAst();
    ast->blockitem = unique_ptr<BaseAst>($1);
    $$ = ast;
} | BlockItem BlockItemList {
    auto ast = new BlockItemListAst();
    ast->blockitem = unique_ptr<BaseAst>($1);
    ast->blockitem_list = unique_ptr<BaseAst>($2);
    $$ = ast;
};

BlockItem: Stmt {
    auto ast = new BlockItemAst();
    ast->stmt = unique_ptr<BaseAst>($1);
    $$ = ast;
} | Decl {
    auto ast = new BlockItemAst();
    ast->decl = unique_ptr<BaseAst>($1);
    $$ = ast;
};

OtherStmt: RETURN Exp ';' {
    auto ast = new OtherStmtAst();
    ast->exp = unique_ptr<BaseAst>($2);
    ast->type = 0;
    $$ = ast;
} | LVal '=' Exp ';' {
    auto ast = new OtherStmtAst();
    ast->lval = unique_ptr<BaseAst>($1);
    ast->exp = unique_ptr<BaseAst>($3);
    ast->type = 1;
    $$ = ast;
} | Block {
    auto ast = new OtherStmtAst();
    ast->block = unique_ptr<BaseAst>($1);
    ast->type = 2;
    $$ = ast;
} | Exp ';' {
    auto ast = new OtherStmtAst();
    ast->exp = unique_ptr<BaseAst>($1);
    ast->type = 3;
    $$ = ast;
} | ';' {
    auto ast = new OtherStmtAst();
    ast->type = 4;
    $$ = ast;
} | WHILE '(' Exp ')' Stmt {
    auto ast = new OtherStmtAst();
    ast->exp = unique_ptr<BaseAst>($3);
    ast->stmt = unique_ptr<BaseAst>($5);
    ast->type = 5;
    $$ = ast;
} | CONTINUE ';' {
    auto ast = new OtherStmtAst();
    ast->type = 6;
    $$ = ast;
} | BREAK ';' {
    auto ast = new OtherStmtAst();
    ast->type = 7;
    $$ = ast;
} | RETURN ';' {
    auto ast = new OtherStmtAst();
    ast->type = 8;
    $$ = ast;
};

Stmt: IfStmt {
    auto ast = new StmtAst();
    ast->if_stmt = unique_ptr<BaseAst>($1);
    $$ = ast;
} | WithElse {
    auto ast = new StmtAst();
    ast->with_else = unique_ptr<BaseAst>($1);
    $$ = ast;
};

IfStmt: IF '(' Exp ')' Stmt {
    auto ast = new IfStmtAst();
    ast->exp = unique_ptr<BaseAst>($3);
    ast->stmt = unique_ptr<BaseAst>($5);
    $$ = ast;
} | IF '(' Exp ')' WithElse ELSE IfStmt {
    auto ast = new IfStmtAst();
    ast->exp = unique_ptr<BaseAst>($3);
    ast->with_else = unique_ptr<BaseAst>($5);
    ast->if_stmt = unique_ptr<BaseAst>($7);
    $$ = ast;
};

WithElse: IF '(' Exp ')' WithElse ELSE WithElse{
    auto ast = new WithElseAst();
    ast->exp = unique_ptr<BaseAst>($3);
    ast->if_withelse = unique_ptr<BaseAst>($5);
    ast->else_withelse = unique_ptr<BaseAst>($7);
    $$ = ast;
} | OtherStmt {
    auto ast = new WithElseAst();
    ast->other_stmt = unique_ptr<BaseAst>($1);
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
} | LVal {
    auto ast = new PrimaryExpAst();
    ast->lval = unique_ptr<BaseAst>($1);
    ast->type = 2;
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
} | IDENT '(' ')' {
    auto ast = new UnaryExpAst();
    ast->ident = *unique_ptr<string>($1);
    ast->type = 2;
    $$ = ast;
} | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAst();
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = unique_ptr<BaseAst>($3);
    ast->type = 2;
    $$ = ast;
};

FuncRParams: FuncRParam {
    auto ast = new FuncRparamsAst();
    ast->func_rparam = unique_ptr<BaseAst>($1);
    $$ = ast;
} | FuncRParam ',' FuncRParams {
    auto ast = new FuncRparamsAst();
    ast->func_rparam = unique_ptr<BaseAst>($1);
    ast->func_rparams = unique_ptr<BaseAst>($3);
    $$ = ast;
};

FuncRParam: Exp {
    auto ast = new FuncRparamAst();
    ast->exp = unique_ptr<BaseAst>($1);
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

Decl: ConstDecl {
    auto ast = new DeclAst();
    ast->const_decl = unique_ptr<BaseAst>($1);
    $$ = ast;
} | VarDecl {
    auto ast = new DeclAst();
    ast->var_decl = unique_ptr<BaseAst>($1);
    $$ = ast;
};

ConstDecl: CONST Btype ConstDefList ';' {
    auto ast = new ConstDeclAst();
    ast->btype = unique_ptr<BaseAst>($2);
    ast->const_def_list = unique_ptr<BaseAst>($3);
    $$ = ast;
};

VarDecl: Btype VarDefList ';' {
    auto ast = new VarDeclAst();
    ast->btype = unique_ptr<BaseAst>($1);
    ast->var_def_list = unique_ptr<BaseAst>($2);
    $$ = ast;
};

VarDefList: VarDef {
    auto ast = new VarDefListAst();
    ast->var_def = unique_ptr<BaseAst>($1);
    $$ = ast;
} | VarDef ',' VarDefList {
    auto ast = new VarDefListAst();
    ast->var_def = unique_ptr<BaseAst>($1);
    ast->var_def_list = unique_ptr<BaseAst>($3);
    $$ = ast;
};

VarDef: IDENT {
    auto ast = new VarDefAst();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
} | IDENT '=' InitVal {
    auto ast = new VarDefAst();
    ast->ident = *unique_ptr<string>($1);
    ast->initval = unique_ptr<BaseAst>($3);
    $$ = ast;
} | IDENT ConstExpList {
    auto ast = new VarDefAst();
    ast->ident = *unique_ptr<string>($1);
    ast->const_exp_list = unique_ptr<BaseAst>($2);
    $$ = ast;
} | IDENT ConstExpList '='  InitVal  {
    auto ast = new VarDefAst();
    ast->ident = *unique_ptr<string>($1);
    ast->const_exp_list = unique_ptr<BaseAst>($2);
    ast->initval = unique_ptr<BaseAst>($4);
    $$ = ast;
};

Btype: INT {
    auto ast = new BTypeAst();
    ast->ident = "i32";
    $$ = ast;
} | VOID {
    auto ast = new BTypeAst();
    ast->ident = "void";
    $$ = ast;
};

ConstDefList: ConstDef {
    auto ast = new ConstDefListAst();
    ast->const_def = unique_ptr<BaseAst>($1);
    $$ = ast;
} | ConstDef ',' ConstDefList {
    auto ast = new ConstDefListAst();
    ast->const_def = unique_ptr<BaseAst>($1);
    ast->const_def_list = unique_ptr<BaseAst>($3);
    $$ = ast;
};

ConstDef: IDENT '=' ConstInitVal {
    auto ast = new ConstDefAst();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAst>($3);
    $$ = ast;
} | IDENT ConstExpList '='  ConstInitVal {
    auto ast = new ConstDefAst();
    ast->ident = *unique_ptr<string>($1);
    ast->const_exp_list = unique_ptr<BaseAst>($2);
    ast->const_init_val = unique_ptr<BaseAst>($4);
    $$ = ast;
};

ConstExpList: '[' ConstExp ']' {
    auto ast = new ConstExpListAst();
    ast->const_exp = unique_ptr<BaseAst>($2);
    $$ = ast;
} | '[' ConstExp ']' ConstExpList {
    auto ast = new ConstExpListAst();
    ast->const_exp = unique_ptr<BaseAst>($2);
    ast->const_exp_list = unique_ptr<BaseAst>($4);
    $$ = ast;
};

ConstInitVal: ConstExp {
    auto ast = new ConstInitValAst();
    ast->const_exp = unique_ptr<BaseAst>($1);
    $$ = ast;
} | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAst();
    ast->const_init_val_list= unique_ptr<BaseAst>($2);
    $$ = ast;
} | '{' '}' {
    auto ast = new ConstInitValAst();
    $$ = ast;
};

ConstInitValList: ConstInitVal {
    auto ast = new ConstInitValListAst();
    ast->const_init_val = unique_ptr<BaseAst>($1);
    $$ = ast;
} | ConstInitVal ',' ConstInitValList {
    auto ast = new ConstInitValListAst();
    ast->const_init_val = unique_ptr<BaseAst>($1);
    ast->const_init_val_list = unique_ptr<BaseAst>($3);
    $$ = ast;
};

InitVal: Exp {
    auto ast = new InitValAst();
    ast->exp = unique_ptr<BaseAst>($1);
    $$ = ast;
} | '{' InitValList '}' {
    auto ast = new InitValAst();
    ast->init_val_list = unique_ptr<BaseAst>($2);
    $$ = ast;
} | '{' '}' {
    auto ast = new InitValAst();
    $$ = ast;
}; //

InitValList: InitVal {
    auto ast = new InitValListAst();
    ast->init_val = unique_ptr<BaseAst>($1);
    $$ = ast;
} | InitVal ',' InitValList {
    auto ast = new InitValListAst();
    ast->init_val = unique_ptr<BaseAst>($1);
    ast->init_val_list = unique_ptr<BaseAst>($3);
    $$ = ast;
};

ConstExp: Exp {
    auto ast = new ConstExpAst();
    ast->exp = unique_ptr<BaseAst>($1);
    $$ = ast;
};

LVal: IDENT {
    auto ast = new LValAst();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
} | IDENT ExpList {
    auto ast = new LValAst();
    ast->ident = *unique_ptr<string>($1);
    ast->exp_list = unique_ptr<BaseAst>($2);
    $$ = ast;
};

ExpList: '[' Exp ']' {
    auto ast = new ExpListAst();
    ast->exp = unique_ptr<BaseAst>($2);
    $$ = ast;
} | '[' Exp ']' ExpList {
    auto ast = new ExpListAst();
    ast->exp = unique_ptr<BaseAst>($2);
    ast->exp_list = unique_ptr<BaseAst>($4);
    $$ = ast;
};



%%

void yyerror(std::unique_ptr<BaseAst>& ast, const char* s) {
    cerr << "error: " << s << endl;
}

