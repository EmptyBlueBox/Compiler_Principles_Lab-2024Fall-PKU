%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  \/\*([^\*]*|[\*]+[^\*\/])*[\*]+\/

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* 运算符 */
ExclusiveUnaryOp       "!"
MulOp         [\*/%]
AddOp         [\+\-]
RelOp         ("<"|">"|"<="|">=")
EqOp          ("=="|"!=")
AndOp         "&&"
OrOp          "||"

%%

// 空白符和注释
{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}  { /* 忽略, 不做任何操作 */ }

// 保留字
"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }

// 标识符
{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

// 整数字面量
{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

// 运算符
{ExclusiveUnaryOp}      { yylval.str_val = new string(yytext); return EXCLUSIVE_UNARY_OP; }
{MulOp}                 { yylval.str_val = new string(yytext); return MUL_OP; }
{AddOp}                 { yylval.str_val = new string(yytext); return ADD_OP; }
{RelOp}                 { yylval.str_val = new string(yytext); return REL_OP; }
{EqOp}                  { yylval.str_val = new string(yytext); return EQ_OP; }
{AndOp}                 { yylval.str_val = new string(yytext); return AND_OP; }
{OrOp}                  { yylval.str_val = new string(yytext); return OR_OP; }

.               { return yytext[0]; }

%%
