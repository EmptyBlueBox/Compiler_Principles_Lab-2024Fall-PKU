#include "include/ast.hpp"
#include "include/define.hpp"

void CompUnitAST::print(const ASTPrintParam &param) const
{
    if (param.mode == MODE_KOOPA || param.mode == MODE_RISC_V)
    {
        func_def->print(param);
    }
    else if (param.mode == MODE_DEBUG)
    {
        *param.output_stream << "CompUnitAST { ";
        func_def->print(param);
        *param.output_stream << " }";
    }
}

void FuncDefAST::print(const ASTPrintParam &param) const
{
    if (param.mode == MODE_KOOPA || param.mode == MODE_RISC_V)
    {
        *param.output_stream << "fun @" << ident << "(): ";
        func_type->print(param);
        *param.output_stream << " {\n";
        block->print(param);
        *param.output_stream << "}\n";
    }
    else if (param.mode == MODE_DEBUG)
    {
        *param.output_stream << "FuncDefAST { ";
        func_type->print(param);
        *param.output_stream << ", " << ident << ", ";
        block->print(param);
        *param.output_stream << " }";
    }
}

void FuncTypeAST::print(const ASTPrintParam &param) const
{
    if (param.mode == MODE_KOOPA || param.mode == MODE_RISC_V)
    {
        if (type == "int")
        {
            *param.output_stream << "i32";
        }
        else if (type == "void")
        {
            *param.output_stream << "void";
        }
    }
    else if (param.mode == MODE_DEBUG)
    {
        *param.output_stream << "FuncTypeAST { " << type << " }";
    }
}

void BlockAST::print(const ASTPrintParam &param) const
{
    if (param.mode == MODE_KOOPA || param.mode == MODE_RISC_V)
    {
        *param.output_stream << "%entry:\n";
        stmt->print(param);
    }
    else if (param.mode == MODE_DEBUG)
    {
        *param.output_stream << "BlockAST { ";
        stmt->print(param);
        *param.output_stream << " }";
    }
}

void StmtAST::print(const ASTPrintParam &param) const
{
    if (param.mode == MODE_KOOPA || param.mode == MODE_RISC_V)
    {
        *param.output_stream << "  ret ";
        *param.output_stream << number;
        *param.output_stream << "\n";
    }
    else if (param.mode == MODE_DEBUG)
    {
        *param.output_stream << "StmtAST { ";
        *param.output_stream << number;
        *param.output_stream << " }";
    }
}
