#pragma once

#include <memory>
#include <string>
#include <iostream>

const std::string AST_MODE_KOOPA = "-koopa";
const std::string AST_MODE_RISC_V = "-riscv";
const std::string AST_MODE_PERFORMANCE = "-perf";
const std::string AST_MODE_DEBUG = "-debug";

// Base class for all AST nodes
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void print(const std::string &mode) const = 0;
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;
    void print(const std::string &mode) const override;
};

class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void print(const std::string &mode) const override;
};

class FuncTypeAST : public BaseAST
{
public:
    std::string type;
    void print(const std::string &mode) const override;
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void print(const std::string &mode) const override;
};

class StmtAST : public BaseAST
{
public:
    int number;
    void print(const std::string &mode) const override;
};
