/**
 * @file include/ast.hpp
 * @brief 定义了前端 AST 的结构体和类；实现了中端 koopa 的输出。
 * @note 前端：通过词法分析和语法分析，将源代码解析成抽象语法树 (AST)。通过语义分析，扫描抽象语法树，检查其是否存在语义错误。
 * @note 中端：将抽象语法树转换为中间表示 (IR)，并在此基础上完成一些机器无关优化。
 * @note 后端：将中间表示转换为目标平台的汇编代码，并在此基础上完成一些机器相关优化。
 * @date 2024-11-13
 */

#pragma once

#include <memory>
#include <string>
#include <deque>
#include <iostream>
#include <sstream>
#include <optional> // --std=c++17 is needed

#include "koopa_util.hpp"

//////////////////////////////////////////
// Program Unit
//////////////////////////////////////////

/**
 * @brief 所有抽象语法树节点的基类。
 * @date 2024-10-27
 */
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    virtual Result print(std::stringstream &output_stream) const = 0;
};

/**
 * @brief 程序单元抽象语法树类。
 * @date 2024-10-27
 */
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def; // 函数定义

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 函数定义抽象语法树类。
 * @date 2024-10-27
 */
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type; // 函数类型
    std::string ident;                  // 函数标识符
    std::unique_ptr<BaseAST> block;     // 函数块

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 函数类型抽象语法树类。
 * @date 2024-10-27
 */
class FuncTypeAST : public BaseAST
{
public:
    std::string type; // 函数的类型

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 块抽象语法树类。
 * @date 2024-10-27
 */
class BlockAST : public BaseAST
{
public:
    std::deque<std::unique_ptr<BaseAST>> block_items;
    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 块中的声明或语句 (Statement)
 * @date 2024-12-22
 */
class BlockItemAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> stmt;
    std::optional<std::unique_ptr<BaseAST>> decl;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 语句抽象语法树类。
 * @note 语句有两种：赋值语句和 return 语句, 这两个语句都必然有表达式, 但是只有赋值语句有左值, 所以 `lval` 是可选的, 而 `exp` 是必选的
 * @date 2024-10-27
 */
class StmtAST : public BaseAST
{
public:
    enum class StmtType
    {
        Assign,
        Expression,
        Block,
        Return,
        If,
        While,
        Break,
        Continue
    };
    StmtType stmt_type;
    std::optional<std::unique_ptr<BaseAST>> lval;              // 语句中的左值
    std::optional<std::unique_ptr<BaseAST>> exp;               // 语句中的表达式
    std::optional<std::unique_ptr<BaseAST>> block;             // 语句中的基本块, 其实是另一个用大括号包裹的语句块
    std::optional<std::unique_ptr<BaseAST>> inside_if_stmt;    // 语句中的 if ... 语句块
    std::optional<std::unique_ptr<BaseAST>> inside_else_stmt;  // 语句中的 else ... 语句块
    std::optional<std::unique_ptr<BaseAST>> inside_while_stmt; // 语句中的 while ... 语句块

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 声明
 * @date 2024-12-22
 */
class DeclAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> const_decl;
    std::optional<std::unique_ptr<BaseAST>> var_decl;
    Result print(std::stringstream &output_stream) const override;
};

//////////////////////////////////////////
// Declaration
//////////////////////////////////////////

/**
 * @brief 声明的类型
 * @date 2024-12-22
 */
class BTypeAST : public BaseAST
{
public:
    std::string type;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 常量声明
 * @date 2024-12-22
 */
class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::deque<std::unique_ptr<BaseAST>> const_defs;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 常量定义
 * @note 常量定义必然有初始值, 所以 `const_init_val` 是必选的
 * @date 2024-12-22
 */
class ConstDefAST : public BaseAST
{
public:
    std::string const_symbol;
    std::unique_ptr<BaseAST> const_init_val;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 常量初始值
 * @date 2024-12-22
 */
class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_exp;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 变量声明
 * @date 2024-12-22
 */
class VarDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::deque<std::unique_ptr<BaseAST>> var_defs;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 变量定义
 * @note 变量定义和常量定义不同, 可以没有初始值, 所以 `var_init_val` 是可选的
 * @date 2024-12-22
 */
class VarDefAST : public BaseAST
{
public:
    std::string var_symbol;
    std::optional<std::unique_ptr<BaseAST>> var_init_val;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 变量初始值
 * @date 2024-12-22
 */
class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    Result print(std::stringstream &output_stream) const override;
};

//////////////////////////////////////////
// Expression and Left Value Definition
//////////////////////////////////////////

/**
 * @brief 表达式抽象语法树类。
 * @date 2024-10-27
 */
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> left_or_exp; // 左操作数或表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 常量表达式抽象语法树类。
 * @date 2024-12-22
 */
class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 左值抽象语法树类。
 * @date 2024-12-22
 */
class LValAST : public BaseAST
{
public:
    std::string left_value_symbol;
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 基本表达式抽象语法树类。
 * @date 2024-10-27
 */
class PrimaryExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> exp;  // 可选的表达式
    std::optional<std::unique_ptr<BaseAST>> lval; // 可选的左值
    std::optional<int> number;                    // 可选的数字

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 一元表达式抽象语法树类。
 * @date 2024-10-27
 */
class UnaryExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> primary_exp; // 可选的基本表达式
    std::optional<std::string> op;                       // 可选的操作符 ("+", "-", "!")
    std::optional<std::unique_ptr<BaseAST>> unary_exp;   // 可选的一元表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 乘法表达式抽象语法树类。
 * @date 2024-10-27
 */
class MulExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> mul_exp;   // 可选的乘法表达式
    std::optional<std::string> op;                     // 可选的操作符 ("*", "/", "%")
    std::optional<std::unique_ptr<BaseAST>> unary_exp; // 可选的一元表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 加法表达式抽象语法树类。
 * @date 2024-10-27
 */
class AddExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> add_exp; // 可选的加法表达式
    std::optional<std::string> op;                   // 可选的操作符 ("+", "-")
    std::optional<std::unique_ptr<BaseAST>> mul_exp; // 可选的乘法表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 关系表达式抽象语法树类。
 * @date 2024-10-27
 */
class RelExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> rel_exp; // 可选的关系表达式
    std::optional<std::string> op;                   // 可选的操作符 ("<", ">", "<=", ">=")
    std::optional<std::unique_ptr<BaseAST>> add_exp; // 可选的加法表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 等式表达式抽象语法树类。
 * @date 2024-10-27
 */
class EqExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> eq_exp;  // 可选的等式表达式
    std::optional<std::string> op;                   // 可选的操作符 ("==", "!=")
    std::optional<std::unique_ptr<BaseAST>> rel_exp; // 可选的关系表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 逻辑与表达式抽象语法树类。
 * @date 2024-10-27
 */
class LAndExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> left_and_exp; // 可选的左与表达式
    std::optional<std::string> op;                        // 可选的操作符 ("&&")
    std::optional<std::unique_ptr<BaseAST>> eq_exp;       // 可选的等式表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 逻辑或表达式抽象语法树类。
 * @date 2024-10-27
 */
class LOrExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> left_or_exp;  // 可选的左或表达式
    std::optional<std::string> op;                        // 可选的操作符 ("||")
    std::optional<std::unique_ptr<BaseAST>> left_and_exp; // 可选的逻辑与表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     */
    Result print(std::stringstream &output_stream) const override;
};
