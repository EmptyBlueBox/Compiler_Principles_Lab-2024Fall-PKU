%code requires {
  #include <memory>
  #include <string>
  #include "include/koopa.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <deque>
#include "include/koopa.hpp"

// declare lexer function and error handling function
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::deque<std::unique_ptr<BaseAST>> *ast_deque_val;
  std::deque<std::string> *str_deque_val;
}


// lexer 返回的所有 token 种类的声明, 终结符的类型为 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> EXCLUSIVE_UNARY_OP MUL_OP ADD_OP REL_OP EQ_OP AND_OP OR_OP // Operators

// 非终结符的类型定义
%type <ast_val> Program CompUnit Block BlockItem Stmt StmtWithElse// Program Unit
%type <ast_val> FuncDef // Function
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal // Declaration
%type <ast_val> Exp ConstExp LVal UnaryExp PrimaryExp MulExp AddExp LOrExp LAndExp RelExp EqExp // Expression
%type <int_val> Number
%type <str_deque_val> MultiFuncFParams
%type <ast_deque_val> ExtendCompUnit ExtendBlockItem ExtendConstDef ExtendVarDef MultiFuncRParams

%%

//////////////////////////////////////////
// Program Unit
//////////////////////////////////////////

Program
  : CompUnit ExtendCompUnit {
    auto program = make_unique<ProgramAST>();
    auto comp_unit = $1;
    program->comp_units.push_back(unique_ptr<BaseAST>(comp_unit));
    for (auto& ptr : *$2) {
      program->comp_units.push_back(std::move(ptr));
    }
    ast = move(program);
  }
  ;

ExtendCompUnit
  : {
    std::deque<unique_ptr<BaseAST>> *deque = new std::deque<unique_ptr<BaseAST>>;
    $$ = deque;
  }
  | CompUnit ExtendCompUnit {
    std::deque<unique_ptr<BaseAST>> *deque = $2;
    deque->push_front(unique_ptr<BaseAST>($1));
    $$ = deque;
  }
  ;

CompUnit
  : FuncDef {
    $$ = $1;
  }
  | Decl {
    $$ = $1;
  }
  ;

FuncDef
  : INT IDENT '(' MultiFuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = FuncDefAST::FuncType::INT;
    ast->ident = *unique_ptr<string>($2);
    ast->func_formal_params = $4;
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  | VOID IDENT '(' MultiFuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = FuncDefAST::FuncType::VOID;
    ast->ident = *unique_ptr<string>($2);
    ast->func_formal_params = $4;
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

MultiFuncFParams
  : {
    $$ = new std::deque<std::string>;
  }
  | INT IDENT {
    auto deque = new std::deque<std::string>;
    deque->push_front(*unique_ptr<string>($2));
    $$ = deque;
  }
  | INT IDENT ',' MultiFuncFParams {
    auto deque = $4;
    deque->push_front(*unique_ptr<string>($2));
    $$ = deque;
  }
  ;

Block
  : '{' BlockItem ExtendBlockItem '}' {
    auto ast = new BlockAST();
    ast->block_items.push_back(unique_ptr<BaseAST>($2));
    for (auto& ptr : *$3) {
      ast->block_items.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  ;

ExtendBlockItem
  : {
    $$ = new std::deque<unique_ptr<BaseAST>>;
  }
  | BlockItem ExtendBlockItem {
    std::deque<unique_ptr<BaseAST>> *deque = $2;
    deque->push_front(unique_ptr<BaseAST>($1));
    $$ = deque;
  }
  ;

BlockItem
  : Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Assign;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Expression;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Expression;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Block;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Return;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Return;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::If;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->inside_if_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' StmtWithElse ELSE Stmt {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::If;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->inside_if_stmt = unique_ptr<BaseAST>($5);
    ast->inside_else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::While;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->inside_while_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Break;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Continue;
    $$ = ast;
  }
  ;

StmtWithElse
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Assign;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Expression;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Expression;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Block;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Return;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Return;
    $$ = ast;
  }
  | IF '(' Exp ')' StmtWithElse ELSE StmtWithElse {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::If;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->inside_if_stmt = unique_ptr<BaseAST>($5);
    ast->inside_else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' StmtWithElse {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::While;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->inside_while_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Break;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->stmt_type = StmtAST::StmtType::Continue;
    $$ = ast;
  }
  ;

//////////////////////////////////////////
// Declaration
//////////////////////////////////////////

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST INT ConstDef ExtendConstDef ';' {
    auto ast = new ConstDeclAST();
    ast->const_defs.push_back(unique_ptr<BaseAST>($3));
    for (auto& ptr : *$4) {
      ast->const_defs.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendConstDef
  : {
    $$ = new std::deque<unique_ptr<BaseAST>>;
  }
  | ',' ConstDef ExtendConstDef {
    std::deque<unique_ptr<BaseAST>> *deque = $3;
    deque->push_front(unique_ptr<BaseAST>($2));
    $$ = deque;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->const_symbol = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : INT VarDef ExtendVarDef ';' {
    auto ast = new VarDeclAST();
    ast->var_defs.push_back(unique_ptr<BaseAST>($2));
    for (auto& ptr : *$3) {
      ast->var_defs.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendVarDef
  : {
    $$ = new std::deque<unique_ptr<BaseAST>>;
  }
  | ',' VarDef ExtendVarDef {
    std::deque<unique_ptr<BaseAST>> *deque = $3;
    deque->push_front(unique_ptr<BaseAST>($2));
    $$ = deque;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->var_symbol = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->var_symbol = *unique_ptr<string>($1);
    ast->var_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

//////////////////////////////////////////
// Expression
//////////////////////////////////////////

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->left_or_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->left_value_symbol = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EXCLUSIVE_UNARY_OP UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | ADD_OP UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' MultiFuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->func_name = *unique_ptr<string>($1);
    std::deque<unique_ptr<BaseAST>> *deque = $3;
    ast->func_real_params = deque;
    $$ = ast;
  }
  ;

MultiFuncRParams
  : {
    $$ = new std::deque<unique_ptr<BaseAST>>;
  }
  | Exp {
    std::deque<unique_ptr<BaseAST>> *deque = new std::deque<unique_ptr<BaseAST>>;
    deque->push_front(unique_ptr<BaseAST>($1));
    $$ = deque;
  }
  | Exp ',' MultiFuncRParams {
    std::deque<unique_ptr<BaseAST>> *deque = $3;
    deque->push_front(unique_ptr<BaseAST>($1));
    $$ = deque;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MUL_OP UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp ADD_OP MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp REL_OP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ_OP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND_OP EqExp {
    auto ast = new LAndExpAST();
    ast->left_and_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->left_and_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OR_OP LAndExp {
    auto ast = new LOrExpAST();
    ast->left_or_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);
    ast->left_and_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
