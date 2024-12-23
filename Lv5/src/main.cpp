#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "include/ast.hpp"
#include "include/backend.hpp"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[])
{
  // parse command line arguments
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // open input file, and specify lexer to read this file
  yyin = fopen(input, "r");
  assert(yyin);

  // parse input file
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  std::stringstream koopa; // 用于存储中端过程输出的 koopa

  freopen(output, "w", stdout);
  if (std::string(mode) == "-koopa")
  {
    ast->print(koopa);
    std::cout << koopa.str();
  }
  else if (std::string(mode) == "-riscv")
  {
    ast->print(koopa);
    backend(koopa.str().c_str());
  }
  fclose(stdout);

  return 0;
}
