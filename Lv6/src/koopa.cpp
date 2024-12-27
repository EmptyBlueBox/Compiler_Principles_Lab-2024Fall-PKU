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
    // 如果 block 没有显式的 ret 指令, 则补上一个 ret 0
    if (!result.control_flow_returned)
    {
        output_stream << "\tret 0" << std::endl;
    }
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
    // 如果没有显式的 return 语句, 则返回 0 , 并且不设置 control_flow_returned 为 true, 以表示没有返回
    return Result();
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

        // 进入 if 语句块, 不用为了特判如下的这种单行语句创建新的符号表, 因为文档里的规约规则没有这种情况, 变量的声明和定义不可能出现在单行 if 中
        // int main()
        // {
        //     int a = 1;
        //     if (a)
        //         int a = 1;
        //     else
        //         int a = 2;
        //     return 0;
        // }
        Result result_if = (*inside_if_stmt)->print(output_stream);

        // 如果 if 语句块显式的返回了, 就不要跳转了, 否则输出这样的 koopa 代码是错误的:
        // fun @main(): i32 {
        // %entry:
        //     br 0, %then_1, %else_1
        // %then_1:
        //     ret 1
        //     jump %end_1
        // %else_1:
        //     ret 2
        //     jump %end_1
        // %end_1:
        // }
        if (!result_if.control_flow_returned)
        {
            output_stream << "\tjump " << end_label << std::endl;
        }

        // else 语句块
        Result result_else = Result();
        if (inside_else_stmt)
        {
            output_stream << else_label << ":" << std::endl;

            // 和 if 同理
            result_else = (*inside_else_stmt)->print(output_stream);

            if (!result_else.control_flow_returned)
            {
                output_stream << "\tjump " << end_label << std::endl;
            }
        }

        // 如果 if 语句块和 else 语句块都返回了, 则注明整个 if ... else ... 语句块返回了
        // 但是为了避免这样的空 %end , 如果已经结束了就不输出 %end 了
        // fun @main(): i32 {
        // %entry:
        // 	   br 0, %then_1, %else_1
        // %then_1:
        //     ret 1
        // %else_1:
        //     ret 2
        // %end_1:
        // }
        Result result = Result();
        if (!result_if.control_flow_returned || !result_else.control_flow_returned)
        {
            output_stream << end_label << ":" << std::endl;
        }
        else
        {
            result.control_flow_returned = true;
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
        // 计算第一个操作数
        Result result_left = (*left_and_exp)->print(output_stream);

        // 短路求值, 根据第一个操作数的形式分类
        // 如果是立即数就可以编译期优化, 完全不用输出 jump 和 br 指令
        // 如果是寄存器就先判断是不是 0, 如果是 0 就跳转到最后, 否则就计算第二个操作数
        if (result_left.type == Result::Type::IMM && result_left.val == 0) // 立即数 0
        {
            return Result(Result::Type::IMM, 0); // 编译期放弃第二个操作数
        }
        else if (result_left.type == Result::Type::IMM && result_left.val != 0) // 立即数非 0, 需要计算第二个操作数
        {
            Result result_right = (*eq_exp)->print(output_stream);
            if (result_right.type == Result::Type::IMM) // 第二个操作数是立即数
            {
                return Result(Result::Type::IMM, 1 && result_right.val);
            }
            else // 第二个操作数是寄存器
            {
                Result temp = Result(Result::Type::REG);
                output_stream << "\t" << temp << " = ne " << result_right << ", 0\n";
                return temp;
            }
        }
        else if (result_left.type == Result::Type::REG) // 如果是寄存器, 不能在编译期完成短路求值, 就需要跳转来完成短路求值, 如果判断寄存器是 0 直接跳转到 and_end_label
        {
            // 每进入一个需要用分支跳转语句达成短路求值的 && 语句, 就设置一个跳转标签
            koopa_context_manager.total_and_statement_count++;

            // 设置跳转标签
            std::string and_second_operator_label = "%and_second_operator_" + std::to_string(koopa_context_manager.total_and_statement_count);
            std::string and_end_label = "%and_end_" + std::to_string(koopa_context_manager.total_and_statement_count);

            // 假设第一个操作数存在了 %1 这个寄存器中, 编译期不知道第二个操作数 %2 是否存在, 所以无法返回 and 表达式整体的答案存在哪里了, 所以需要结果存在内存中以保证可以修改
            std::string and_result_in_memory = "@and_result_in_memory_" + std::to_string(koopa_context_manager.total_and_statement_count);

            // 如果第一个操作数是 1, 则跳转到 and_second_operator_label 看看第二个操作数是否是 1, 否则跳转到 and_end_label
            Result temp_1 = Result(Result::Type::REG);
            output_stream << "\t" << temp_1 << " = ne " << result_left << ", 0\n";
            output_stream << "\t" << and_result_in_memory << " = alloc i32\n";
            output_stream << "\tstore " << temp_1 << ", " << and_result_in_memory << "\n";
            output_stream << "\tbr " << temp_1 << ", " << and_second_operator_label << ", " << and_end_label << "\n";

            // 输出没有短路求值的控制流 label
            output_stream << and_second_operator_label << ":" << std::endl;

            // 计算第二个操作数
            Result result_right = (*eq_exp)->print(output_stream);
            Result temp_2 = Result(Result::Type::REG);
            Result temp_3 = Result(Result::Type::REG);
            output_stream << "\t" << temp_2 << " = ne " << result_right << ", 0\n";
            output_stream << "\t" << temp_3 << " = and " << temp_1 << ", " << temp_2 << "\n";
            output_stream << "\tstore " << temp_3 << ", " << and_result_in_memory << "\n";
            output_stream << "\tjump " << and_end_label << "\n";

            // 输出短路求值之后的控制流合并 label
            output_stream << and_end_label << ":" << std::endl;

            // 把结果从内存中读取到寄存器中
            Result result = Result(Result::Type::REG);
            output_stream << "\t" << result << " = load " << and_result_in_memory << "\n";
            return result;
        }
        else
        {
            throw std::runtime_error("LAndExpAST::print: invalid first operand of logical AND expression");
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

        if (result_left.type == Result::Type::IMM && result_left.val != 0) // 立即数非 0
        {
            return Result(Result::Type::IMM, 1);
        }
        else if (result_left.type == Result::Type::IMM && result_left.val == 0) // 立即数 0
        {
            Result result_right = (*left_and_exp)->print(output_stream);
            if (result_right.type == Result::Type::IMM)
            {
                return Result(Result::Type::IMM, 0 || result_right.val);
            }
            else
            {
                Result temp = Result(Result::Type::REG);
                output_stream << "\t" << temp << " = ne " << result_right << ", 0\n";
                return temp;
            }
        }
        else if (result_left.type == Result::Type::REG) // 如果是寄存器, 不能在编译期完成短路求值, 就需要跳转来完成短路求值, 如果判断寄存器是 0 直接跳转到 or_end_label
        {
            // 每进入一个需要用分支跳转语句达成短路求值的 || 语句, 就设置一个跳转标签
            koopa_context_manager.total_or_statement_count++;

            // 设置跳转标签
            std::string or_second_operator_label = "%or_second_operator_" + std::to_string(koopa_context_manager.total_or_statement_count);
            std::string or_end_label = "%or_end_" + std::to_string(koopa_context_manager.total_or_statement_count);

            // 假设第一个操作数存在了 %1 这个寄存器中, 编译期不知道第二个操作数 %2 是否存在, 所以无法返回 or 表达式整体的答案存在哪里了, 所以需要结果存在内存中以保证可以修改
            std::string or_result_in_memory = "@or_result_in_memory_" + std::to_string(koopa_context_manager.total_or_statement_count);

            // 如果第一个操作数是 0, 则跳转到 or_second_operator_label 看看第二个操作数是否是 0, 否则跳转到 or_end_label
            Result temp_1 = Result(Result::Type::REG);
            output_stream << "\t" << temp_1 << " = ne " << result_left << ", 0\n";
            output_stream << "\t" << or_result_in_memory << " = alloc i32\n";
            output_stream << "\tstore " << temp_1 << ", " << or_result_in_memory << "\n";
            output_stream << "\tbr " << temp_1 << ", " << or_end_label << ", " << or_second_operator_label << "\n";

            // 输出没有短路求值的控制流 label
            output_stream << or_second_operator_label << ":" << std::endl;

            // 计算第二个操作数
            Result result_right = (*left_and_exp)->print(output_stream);
            Result temp_2 = Result(Result::Type::REG);
            Result temp_3 = Result(Result::Type::REG);
            output_stream << "\t" << temp_2 << " = ne " << result_right << ", 0\n";
            output_stream << "\t" << temp_3 << " = or " << temp_1 << ", " << temp_2 << "\n";
            output_stream << "\tstore " << temp_3 << ", " << or_result_in_memory << "\n";
            output_stream << "\tjump " << or_end_label << "\n";

            // 输出短路求值之后的控制流合并 label
            output_stream << or_end_label << ":" << std::endl;

            // 把结果从内存中读取到寄存器中
            Result result = Result(Result::Type::REG);
            output_stream << "\t" << result << " = load " << or_result_in_memory << "\n";
            return result;
        }
        else
        {
            throw std::runtime_error("LOrExpAST::print: invalid first operand of logical OR expression");
        }
    }
    else
    {
        throw std::runtime_error("LOrExpAST::print: invalid logical OR expression");
    }
}
