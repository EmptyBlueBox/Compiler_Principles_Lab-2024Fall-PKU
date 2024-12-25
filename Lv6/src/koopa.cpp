#include "include/koopa.hpp"

// 定义并初始化, 应该在 CPP 文件中否则会造成重复定义的链接器问题, 最开始计算的数据没有存储在任何寄存器中, 所以初始化为 -1
int Result::current_symbol_index = -1;

// 全局符号表
KoopaContextManager koopa_context_manager;

//////////////////////////////////////////
// Program Unit
//////////////////////////////////////////

Result CompUnitAST::print(std::stringstream &output_stream) const
{
    Result result = func_def->print(output_stream);
    return result;
}

Result FuncDefAST::print(std::stringstream &output_stream) const
{
    output_stream << "fun @" << ident << "(): ";
    func_type->print(output_stream);
    output_stream << " {" << std::endl;
    output_stream << "%entry:" << std::endl;
    Result result = block->print(output_stream);
    output_stream << "}" << std::endl;
    return result;
}

Result FuncTypeAST::print(std::stringstream &output_stream) const
{
    if (type == "int")
    {
        output_stream << "i32";
    }
    else
    {
        throw std::runtime_error("FuncTypeAST::print: invalid function type");
    }
    return Result();
}

Result BlockAST::print(std::stringstream &output_stream) const
{
    koopa_context_manager.new_symbol_table_hierarchy();
    for (const auto &item : block_items)
    {
        Result result = item->print(output_stream);
        if (result.control_flow_returned)
        {
            koopa_context_manager.delete_symbol_table_hierarchy();
            return result;
        }
    }
    koopa_context_manager.delete_symbol_table_hierarchy();
    return Result(); // 如果没有显式的 return 语句, 则返回 0
}

Result BlockItemAST::print(std::stringstream &output_stream) const
{
    if (stmt && !decl)
    {
        return (*stmt)->print(output_stream);
    }
    else if (!stmt && decl)
    {
        return (*decl)->print(output_stream);
    }
    else
    {
        throw std::runtime_error("BlockItemAST::print: invalid block item");
    }
}

Result StmtAST::print(std::stringstream &output_stream) const
{
    if (stmt_type == StmtType::Assign)
    {
        if (lval && exp && !block)
        {
            // 这里不能调用 lval->print , 因为这里的 lval 不应该作为一个引用 (左值) 出现, 这里需要一个字符串来判断符号是否已经存在
            std::string symbol_name = ((LValAST *)(*lval).get())->left_value_symbol;
            Result result = (*exp)->print(output_stream);
            Symbol symbol = koopa_context_manager.name_to_symbol(symbol_name);
            if (symbol.type == Symbol::Type::VAL)
            {
                throw std::runtime_error("StmtAST::print: assign to a constant");
            }
            std::string suffix = std::to_string(symbol.val);
            std::string symbol_name_with_suffix = symbol_name + "_" + suffix;
            output_stream << "\tstore " << result << ", @" << symbol_name_with_suffix << "\n";
            return Result();
        }
        else
        {
            throw std::runtime_error("StmtAST::print: invalid assign statement");
        }
    }
    else if (stmt_type == StmtType::Return)
    {
        if (!lval && exp && !block)
        {
            Result result = (*exp)->print(output_stream);
            output_stream << "\tret " << result << "\n";
            result.control_flow_returned = true;
            return result;
        }
        else if (!lval && !exp && !block)
        {
            output_stream << "\tret\n";
            Result result = Result();
            result.control_flow_returned = true;
            return result;
        }
        else
        {
            throw std::runtime_error("StmtAST::print: invalid return statement");
        }
    }
    else if (stmt_type == StmtType::Expression)
    {
        if (!lval && exp && !block)
        {
            (*exp)->print(output_stream);
            return Result(); // 表达式语句不会返回任何值
        }
        else if (!lval && !exp && !block)
        {
            return Result(); // 空表达式语句不会返回任何值
        }
        else
        {
            throw std::runtime_error("StmtAST::print: invalid expression statement");
        }
    }
    else if (stmt_type == StmtType::Block)
    {
        if (!lval && !exp && block)
        {
            Result result = (*block)->print(output_stream);
            return result;
        }
        else
        {
            throw std::runtime_error("StmtAST::print: invalid block statement");
        }
    }
    else if (stmt_type == StmtType::If)
    {
        koopa_context_manager.new_symbol_table_hierarchy();
        koopa_context_manager.total_if_else_statement_count++;
        std::string then_label = "%then_" + std::to_string(koopa_context_manager.total_if_else_statement_count);
        std::string else_label = "%else_" + std::to_string(koopa_context_manager.total_if_else_statement_count);
        std::string end_label = "%end_" + std::to_string(koopa_context_manager.total_if_else_statement_count);

        Result exp_result = (*exp)->print(output_stream);
        if (!inside_if_stmt && !inside_else_stmt)
        {
            throw std::runtime_error("StmtAST::print: invalid if statement, there's no if");
        }

        output_stream << "\tbr " << exp_result << ", " << then_label << ", " << (inside_else_stmt ? else_label : end_label) << std::endl;

        // if 语句块
        output_stream << then_label << ":" << std::endl;
        Result result_if = (*inside_if_stmt)->print(output_stream);
        if (!result_if.control_flow_returned)
        {
            output_stream << "\tjump " << end_label << std::endl;
        }

        // else 语句块
        Result result_else = Result();
        if (inside_else_stmt)
        {
            output_stream << else_label << ":" << std::endl;
            result_else = (*inside_else_stmt)->print(output_stream);
            if (!result_else.control_flow_returned)
            {
                output_stream << "\tjump " << end_label << std::endl;
            }
        }
        output_stream << end_label << ":" << std::endl;
        koopa_context_manager.delete_symbol_table_hierarchy();

        // 如果 if 语句块和 else 语句块都返回了, 则注明整个 if ... else ... 语句块返回了
        // 但是为了避免这样的空 %end , 额外补充一个空 ret 语句
        // fun @main(): i32 {
        // %entry:
        // 	br 0, %then_1, %else_1
        // %then_1:
        // 	ret 1
        // %else_1:
        // 	ret 2
        // %end_1:
        // }
        Result result = Result();
        if (result_if.control_flow_returned && result_else.control_flow_returned)
        {
            result.control_flow_returned = true;
            output_stream << "\tret\n";
        }
        return result;
    }
    else
    {
        throw std::runtime_error("StmtAST::print: invalid statement");
    }
}

Result DeclAST::print(std::stringstream &output_stream) const
{
    if (const_decl)
    {
        (*const_decl)->print(output_stream);
    }
    else if (var_decl)
    {
        (*var_decl)->print(output_stream);
    }
    else
    {
        throw std::runtime_error("DeclAST::print: invalid declaration");
    }
    return Result(); // 返回空结果, 为什么不返回调用 print 的返回值? 因为我们的先验知识 (语义规范) 告诉我们, 声明语句不会返回任何值
}

//////////////////////////////////////////
// Declaration
//////////////////////////////////////////

Result BTypeAST::print(std::stringstream &output_stream) const
{
    output_stream << "i32";
    return Result();
}

Result ConstDeclAST::print(std::stringstream &output_stream) const
{
    for (const auto &item : const_defs)
    {
        item->print(output_stream);
    }
    return Result();
}

Result ConstDefAST::print(std::stringstream &output_stream) const
{
    Result value_result = const_init_val->print(output_stream);
    koopa_context_manager.insert_symbol(const_symbol, Symbol(Symbol::Type::VAL, value_result.val));
    return Result();
}

Result ConstInitValAST::print(std::stringstream &output_stream) const
{
    return const_exp->print(output_stream);
}

Result VarDeclAST::print(std::stringstream &output_stream) const
{
    for (const auto &item : var_defs)
    {
        item->print(output_stream);
    }
    return Result();
}

Result VarDefAST::print(std::stringstream &output_stream) const
{
    if (var_init_val)
    {
        Result value_result = (*var_init_val)->print(output_stream);
        koopa_context_manager.insert_symbol(var_symbol, Symbol(Symbol::Type::VAR, value_result.val));
        std::string symbol_name = var_symbol;
        std::string suffix = std::to_string(koopa_context_manager.name_to_symbol(symbol_name).val);
        std::string symbol_name_with_suffix = symbol_name + "_" + suffix;
        if (!koopa_context_manager.is_symbol_allocated_in_this_level(symbol_name))
        {
            output_stream << "\t@" << symbol_name_with_suffix << " = alloc i32\n"; // TODO: 这里需要根据类型分配空间, 但是类型保存在上一层 VarDeclAST 中的 btype 中, 访问不到, 但是暂时只需要处理 int 类型, 所以 hard code 即可
        }
        koopa_context_manager.set_symbol_allocated_in_this_level(symbol_name);
        output_stream << "\tstore " << value_result << ", @" << symbol_name_with_suffix << "\n";
    }
    else
    {
        koopa_context_manager.insert_symbol(var_symbol, Symbol(Symbol::Type::VAR, 0));
        std::string symbol_name = var_symbol;
        std::string suffix = std::to_string(koopa_context_manager.name_to_symbol(symbol_name).val);
        std::string symbol_name_with_suffix = symbol_name + "_" + suffix;
        if (!koopa_context_manager.is_symbol_allocated_in_this_level(symbol_name))
        {
            output_stream << "\t@" << symbol_name_with_suffix << " = alloc i32\n"; // TODO: 这里需要根据类型分配空间, 但是类型保存在上一层 VarDeclAST 中的 btype 中, 访问不到, 但是暂时只需要处理 int 类型, 所以 hard code 即可
        }
        koopa_context_manager.set_symbol_allocated_in_this_level(symbol_name);
    }
    return Result();
}

Result InitValAST::print(std::stringstream &output_stream) const
{
    return exp->print(output_stream);
}

//////////////////////////////////////////
// Expression and Left Value
//////////////////////////////////////////

Result ExpAST::print(std::stringstream &output_stream) const
{
    Result result = left_or_exp->print(output_stream);
    return result;
}

Result ConstExpAST::print(std::stringstream &output_stream) const
{
    return exp->print(output_stream);
}

Result LValAST::print(std::stringstream &output_stream) const
{
    if (koopa_context_manager.name_to_symbol(left_value_symbol).type == Symbol::Type::VAR)
    {
        std::string symbol_name = left_value_symbol;
        std::string suffix = std::to_string(koopa_context_manager.name_to_symbol(symbol_name).val);
        std::string symbol_name_with_suffix = symbol_name + "_" + suffix;
        Result result = Result(Result::Type::REG);
        output_stream << "\t" << result << " = load @" << symbol_name_with_suffix << "\n";
        return result;
    }
    else if (koopa_context_manager.name_to_symbol(left_value_symbol).type == Symbol::Type::VAL)
    {
        Result result = Result(Result::Type::IMM, koopa_context_manager.name_to_symbol(left_value_symbol).val);
        return result;
    }
    else
    {
        throw std::runtime_error("LValAST::print: identifier is not a variable");
    }
}

Result PrimaryExpAST::print(std::stringstream &output_stream) const
{
    if (exp && !number && !lval)
    {
        return (*exp)->print(output_stream);
    }
    else if (!exp && number && !lval)
    {
        Result result = Result(Result::Type::IMM, *number);
        return result;
    }
    else if (!exp && !number && lval)
    {
        return (*lval)->print(output_stream);
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
        if (unary_result.type == Result::Type::IMM)
        {
            if (*op == "+")
            {
                return Result(Result::Type::IMM, unary_result.val);
            }
            else if (*op == "-")
            {
                return Result(Result::Type::IMM, -unary_result.val);
            }
            else if (*op == "!")
            {
                return Result(Result::Type::IMM, !unary_result.val);
            }
            else
            {
                throw std::runtime_error("UnaryExpAST::print: invalid unary operator when unary_result is immediate");
            }
        }
        else
        {
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
                throw std::runtime_error("UnaryExpAST::print: invalid unary operator when unary_result is not immediate");
            }
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            if (*op == "*")
            {
                return Result(Result::Type::IMM, result_left.val * result_right.val);
            }
            else if (*op == "/")
            {
                return Result(Result::Type::IMM, result_left.val / result_right.val);
            }
            else if (*op == "%")
            {
                return Result(Result::Type::IMM, result_left.val % result_right.val);
            }
            else
            {
                throw std::runtime_error("MulExpAST::print: invalid mul operator when both operands are immediate");
            }
        }
        else
        {
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
                throw std::runtime_error("MulExpAST::print: invalid mul operator when one of the operands  not immediate");
            }
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            if (*op == "+")
            {
                return Result(Result::Type::IMM, result_left.val + result_right.val);
            }
            else if (*op == "-")
            {
                return Result(Result::Type::IMM, result_left.val - result_right.val);
            }
            else
            {
                throw std::runtime_error("AddExpAST::print: invalid add operator when both operands are immediate");
            }
        }
        else
        {
            Result result = Result(Result::Type::REG);
            if (*op == "+")
            {
                output_stream << "\t" << result << " = add " << result_left << ", " << result_right << "\n";
            }
            else if (*op == "-")
            {
                output_stream << "\t" << result << " = sub " << result_left << ", " << result_right << "\n";
            }
            else
            {
                throw std::runtime_error("AddExpAST::print: invalid add operator when one of the operands is not immediate");
            }
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            if (*op == "<")
            {
                return Result(Result::Type::IMM, result_left.val < result_right.val);
            }
            else if (*op == ">")
            {
                return Result(Result::Type::IMM, result_left.val > result_right.val);
            }
            else if (*op == "<=")
            {
                return Result(Result::Type::IMM, result_left.val <= result_right.val);
            }
            else if (*op == ">=")
            {
                return Result(Result::Type::IMM, result_left.val >= result_right.val);
            }
            else
            {
                throw std::runtime_error("RelExpAST::print: invalid relational operator when both operands are immediate");
            }
        }
        else
        {
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
                throw std::runtime_error("RelExpAST::print: invalid relational operator when one of the operands is not immediate");
            }
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            if (*op == "==")
            {
                return Result(Result::Type::IMM, result_left.val == result_right.val);
            }
            else if (*op == "!=")
            {
                return Result(Result::Type::IMM, result_left.val != result_right.val);
            }
            else
            {
                throw std::runtime_error("EqExpAST::print: invalid equality operator when both operands are immediate");
            }
        }
        else
        {
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
                throw std::runtime_error("EqExpAST::print: invalid equality operator when one of the operands is not immediate");
            }
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            return Result(Result::Type::IMM, result_left.val && result_right.val);
        }
        else
        {
            Result temp_1 = Result(Result::Type::REG);
            Result temp_2 = Result(Result::Type::REG);
            Result result = Result(Result::Type::REG);
            output_stream << "\t" << temp_1 << " = ne " << result_left << ", 0\n";
            output_stream << "\t" << temp_2 << " = ne " << result_right << ", 0\n";
            output_stream << "\t" << result << " = and " << temp_1 << ", " << temp_2 << "\n";
            return result;
        }
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
        if (result_left.type == Result::Type::IMM && result_right.type == Result::Type::IMM)
        {
            return Result(Result::Type::IMM, result_left.val || result_right.val);
        }
        else
        {
            Result temp = Result(Result::Type::REG);
            Result result = Result(Result::Type::REG);
            output_stream << "\t" << temp << " = or " << result_left << ", " << result_right << "\n";
            output_stream << "\t" << result << " = ne " << temp << ", 0\n";
            return result;
        }
    }
    else
    {
        throw std::runtime_error("LOrExpAST::print: invalid logical OR expression");
    }
}
