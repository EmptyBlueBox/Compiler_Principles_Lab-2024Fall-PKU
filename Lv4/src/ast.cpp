#include "include/ast.hpp"
#include "include/define.hpp"

// 定义并初始化, 应该在 CPP 文件中否则会造成重复定义的链接器问题, 最开始计算的数据没有存储在任何寄存器中, 所以初始化为 -1
int Result::current_symbol_index = -1;

Result CompUnitAST::print(std::stringstream &output_stream) const
{
    Result result = func_def->print(output_stream);
    return result;
}

Result FuncDefAST::print(std::stringstream &output_stream) const
{
    output_stream << "fun @" << ident << "(): ";
    func_type->print(output_stream);
    output_stream << " {\n";
    Result result = block->print(output_stream);
    output_stream << "}\n";
    return result;
}

Result FuncTypeAST::print(std::stringstream &output_stream) const
{
    if (type == "int")
    {
        output_stream << "i32";
    }
    else if (type == "void")
    {
        output_stream << "void";
    }
    return Result();
}

Result BlockAST::print(std::stringstream &output_stream) const
{
    output_stream << "%entry:\n";
    Result result = stmt->print(output_stream);
    return result;
}

Result StmtAST::print(std::stringstream &output_stream) const
{
    Result result = exp->print(output_stream);
    output_stream << "\tret " << result << "\n";
    return result;
}

Result ExpAST::print(std::stringstream &output_stream) const
{
    Result result = left_or_exp->print(output_stream);
    return result;
}

Result PrimaryExpAST::print(std::stringstream &output_stream) const
{
    if (exp && !number)
    {
        return (*exp)->print(output_stream);
    }
    else if (!exp && number)
    {
        Result result = Result(Result::Type::IMM, *number);
        return result;
    }
    else
    {
        throw std::runtime_error("PrimaryExpAST::print: invalid primary expression");
    }
}

Result UnaryExpAST::print(std::stringstream &output_stream) const
{
    if (primary_exp && !op && !unary_exp)
    {
        return (*primary_exp)->print(output_stream);
    }
    else if (!primary_exp && op && unary_exp)
    {
        Result unary_result = (*unary_exp)->print(output_stream);
        Result result = Result(Result::Type::REG);
        if (*op == "+")
        {
            output_stream << "\t" << result << " = add 0, " << unary_result << "\n";
        }
        else if (*op == "-")
        {
            output_stream << "\t" << result << " = sub 0, " << unary_result << "\n";
        }
        else if (*op == "!")
        {
            output_stream << "\t" << result << " = eq 0, " << unary_result << "\n";
        }
        else
        {
            throw std::runtime_error("UnaryExpAST::print: invalid unary operator");
        }
        return result;
    }
    else
    {
        throw std::runtime_error("UnaryExpAST::print: invalid unary expression");
    }
}

Result MulExpAST::print(std::stringstream &output_stream) const
{
    if (!mul_exp && !op && unary_exp)
    {
        return (*unary_exp)->print(output_stream);
    }
    else if (mul_exp && op && unary_exp)
    {
        Result result_left = (*mul_exp)->print(output_stream);
        Result result_right = (*unary_exp)->print(output_stream);
        Result result = Result(Result::Type::REG);
        if (*op == "*")
        {
            output_stream << "\t" << result << " = mul " << result_left << ", " << result_right << "\n";
        }
        else if (*op == "/")
        {
            output_stream << "\t" << result << " = div " << result_left << ", " << result_right << "\n";
        }
        else if (*op == "%")
        {
            output_stream << "\t" << result << " = mod " << result_left << ", " << result_right << "\n";
        }
        else
        {
            throw std::runtime_error("MulExpAST::print: invalid mul operator");
        }
        return result;
    }
    else
    {
        throw std::runtime_error("MulExpAST::print: invalid mul expression");
    }
}

Result AddExpAST::print(std::stringstream &output_stream) const
{
    if (!add_exp && !op && mul_exp)
    {
        return (*mul_exp)->print(output_stream);
    }
    else if (add_exp && op && mul_exp)
    {
        Result result_left = (*add_exp)->print(output_stream);
        Result result_right = (*mul_exp)->print(output_stream);
        Result result = Result(Result::Type::REG);
        if (*op == "+")
        {
            output_stream << "\t" << result << " = add " << result_left << ", " << result_right << "\n";
        }
        else if (*op == "-")
        {
            output_stream << "\t" << result << " = sub " << result_left << ", " << result_right << "\n";
        }
        return result;
    }
    else
    {
        throw std::runtime_error("AddExpAST::print: invalid add expression");
    }
}

Result RelExpAST::print(std::stringstream &output_stream) const
{
    if (!rel_exp && !op && add_exp)
    {
        return (*add_exp)->print(output_stream);
    }
    else if (rel_exp && op && add_exp)
    {
        Result result_left = (*rel_exp)->print(output_stream);
        Result result_right = (*add_exp)->print(output_stream);
        Result result = Result(Result::Type::REG);
        if (*op == "<")
        {
            output_stream << "\t" << result << " = lt " << result_left << ", " << result_right << "\n";
        }
        else if (*op == ">")
        {
            output_stream << "\t" << result << " = gt " << result_left << ", " << result_right << "\n";
        }
        else if (*op == "<=")
        {
            output_stream << "\t" << result << " = le " << result_left << ", " << result_right << "\n";
        }
        else if (*op == ">=")
        {
            output_stream << "\t" << result << " = ge " << result_left << ", " << result_right << "\n";
        }
        else
        {
            throw std::runtime_error("RelExpAST::print: invalid relational operator");
        }
        return result;
    }
    else
    {
        throw std::runtime_error("RelExpAST::print: invalid relational expression");
    }
}

Result EqExpAST::print(std::stringstream &output_stream) const
{
    if (!eq_exp && !op && rel_exp)
    {
        return (*rel_exp)->print(output_stream);
    }
    else if (eq_exp && op && rel_exp)
    {
        Result result_left = (*eq_exp)->print(output_stream);
        Result result_right = (*rel_exp)->print(output_stream);
        Result result = Result(Result::Type::REG);
        if (*op == "==")
        {
            output_stream << "\t" << result << " = eq " << result_left << ", " << result_right << "\n";
        }
        else if (*op == "!=")
        {
            output_stream << "\t" << result << " = ne " << result_left << ", " << result_right << "\n";
        }
        else
        {
            throw std::runtime_error("EqExpAST::print: invalid equality operator");
        }
        return result;
    }
    else
    {
        throw std::runtime_error("EqExpAST::print: invalid equality expression");
    }
}

Result LAndExpAST::print(std::stringstream &output_stream) const
{
    if (!left_and_exp && !op && eq_exp)
    {
        return (*eq_exp)->print(output_stream);
    }
    else if (left_and_exp && op && eq_exp)
    {
        Result result_left = (*left_and_exp)->print(output_stream);
        Result result_right = (*eq_exp)->print(output_stream);
        Result temp_1 = Result(Result::Type::REG);
        Result temp_2 = Result(Result::Type::REG);
        Result result = Result(Result::Type::REG);
        output_stream << "\t" << temp_1 << " = ne " << result_left << ", 0\n";
        output_stream << "\t" << temp_2 << " = ne " << result_right << ", 0\n";
        output_stream << "\t" << result << " = and " << temp_1 << ", " << temp_2 << "\n";
        return result;
    }
    else
    {
        throw std::runtime_error("LAndExpAST::print: invalid logical AND expression");
    }
}

Result LOrExpAST::print(std::stringstream &output_stream) const
{
    if (!left_or_exp && !op && left_and_exp)
    {
        return (*left_and_exp)->print(output_stream);
    }
    else if (left_or_exp && op && left_and_exp)
    {
        Result result_left = (*left_or_exp)->print(output_stream);
        Result result_right = (*left_and_exp)->print(output_stream);
        Result temp = Result(Result::Type::REG);
        Result result = Result(Result::Type::REG);
        output_stream << "\t" << temp << " = or " << result_left << ", " << result_right << "\n";
        output_stream << "\t" << result << " = ne " << temp << ", 0\n";
        return result;
    }
    else
    {
        throw std::runtime_error("LOrExpAST::print: invalid logical OR expression");
    }
}
