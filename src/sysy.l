%option noyywrap
%option nounput
%option noinput

%{
    #include <cstdlib>
    #include <string>

    #include "sysy.tab.hpp"

    using namespace std;
%}

WhiteSpace [ \t\n\r]*
LineComment "//".*|"/*"([^\*]*|[\*]+[^\*\/])*[\*]+\/
Identifier [a-zA-Z_][a-zA-Z0-9_]*
Decimal [1-9][0-9]*
Octal 0[0-7]*
Hexadecimal 0[xX][0-9a-fA-F]+
ADD_OP [+\-]
MUL_OP [*\/%]
CMP_OP <|>|<=|>=
EQ_OP ==|!=
LAND_OP &&
LOR_OP \|\|
OTHER_OP !

%%

{WhiteSpace} { /* ignore */ }
{LineComment} { /* ignore */ }

"int" { return INT; }
"return" { return RETURN; }
"const" {return CONST; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }
"break" { return BREAK; }
"continue" { return CONTINUE; }

{Identifier} { yylval.str_val = new string(yytext); return IDENT; }
{Decimal} { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal} { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal} { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{ADD_OP} { yylval.str_val = new string(yytext); return ADD_OP; }
{MUL_OP} { yylval.str_val = new string(yytext); return MUL_OP; }
{CMP_OP} { yylval.str_val = new string(yytext); return CMP_OP; }
{EQ_OP} { yylval.str_val = new string(yytext); return EQ_OP; }
{LAND_OP} { yylval.str_val = new string(yytext); return LAND_OP; }
{LOR_OP} { yylval.str_val = new string(yytext); return LOR_OP; }
{OTHER_OP} { yylval.str_val = new string(yytext); return OTHER_OP; }

. { return yytext[0]; }

%%
