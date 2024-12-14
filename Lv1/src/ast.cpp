#include "include/ast.hpp"

void CompUnitAST::print(const std::string &mode) const
{
    if (mode == AST_MODE_KOOPA)
    {
        func_def->print(mode);
    }
    else if (mode == AST_MODE_DEBUG)
    {
        std::cout << "CompUnitAST { ";
        func_def->print(mode);
        std::cout << " }";
    }
}

void FuncDefAST::print(const std::string &mode) const
{
    if (mode == AST_MODE_KOOPA)
    {
        std::cout << "fun @" << ident << "(): ";
        func_type->print(mode);
        std::cout << " {\n";
        block->print(mode);
        std::cout << "}\n";
    }
    else if (mode == AST_MODE_DEBUG)
    {
        std::cout << "FuncDefAST { ";
        func_type->print(mode);
        std::cout << ", " << ident << ", ";
        block->print(mode);
        std::cout << " }";
    }
}

void FuncTypeAST::print(const std::string &mode) const
{
    if (mode == AST_MODE_KOOPA)
    {
        if (type == "int")
        {
            std::cout << "i32";
        }
        else if (type == "void")
        {
            std::cout << "void";
        }
    }
    else if (mode == AST_MODE_DEBUG)
    {
        std::cout << "FuncTypeAST { " << type << " }";
    }
}

void BlockAST::print(const std::string &mode) const
{
    if (mode == AST_MODE_KOOPA)
    {
        std::cout << "%entry:\n";
        stmt->print(mode);
    }
    else if (mode == AST_MODE_DEBUG)
    {
        std::cout << "BlockAST { ";
        stmt->print(mode);
        std::cout << " }";
    }
}

void StmtAST::print(const std::string &mode) const
{
    if (mode == AST_MODE_KOOPA)
    {
        std::cout << "  ret ";
        std::cout << number;
        std::cout << "\n";
    }
    else if (mode == AST_MODE_DEBUG)
    {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout << " }";
    }
}
