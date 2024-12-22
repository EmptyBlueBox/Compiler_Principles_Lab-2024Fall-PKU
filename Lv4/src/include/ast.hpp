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
#include <iostream>
#include <sstream>
#include <optional> // --std=c++17

/**
 * @brief 用于存储计算结果的类，可以是符号或立即数。
 * @note 如果当前函数会产生一个计算结果, 那么这个计算结果会存储在返回的 `Result` 类型的变量中
 * @note 比如 `PrimaryExpAST` 的 `print` 函数, 当它是从数字规约而来时, 它的 `Result` 变量会被初始化为立即数, 返回 `Result(Result::Type::IMM, *number)` 这样一个变量
 * @note 如果当前函数不会产生计算结果, 那么返回的 `Result` 变量会被初始化为立即数 0
 * @date 2024-11-27
 */
class Result
{
public:
    /**
     * @brief 当前计算值，存储在 `%current_value_symbol_index` 符号中。
     * @date 2024-11-27
     */
    static int current_symbol_index;

    enum class Type
    {
        IMM, // 立即数
        REG  // 寄存器
    };
    Type type; // 结果的类型
    int val;   // 结果的值

    // 默认构造函数，初始化为立即数 0, 没有用到的地方
    Result() : type(Type::IMM), val(0) {}

    // 带有指定类型的构造函数, 主要用来初始化寄存器
    Result(Type type) : type(type), val(0)
    {
        if (type == Type::REG)
        {
            val = ++current_symbol_index;
        }
    }

    // 带有指定类型和值的构造函数, 主要用来初始化立即数
    Result(Type type, int val) : type(type), val(val)
    {
        if (type == Type::REG)
        {
            val = ++current_symbol_index;
        }
    }

    // 重载 <<
    friend std::ostream &operator<<(std::ostream &os, const Result &result)
    {
        os << (result.type == Result::Type::REG ? "%" : "") << result.val;
        return os;
    }
};

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

//////////////////////////////////////////
// 程序单元
//////////////////////////////////////////

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
    std::unique_ptr<BaseAST> stmt; // 块中的语句

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

/**
 * @brief 语句抽象语法树类。
 * @date 2024-10-27
 */
class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp; // 语句中的表达式

    /**
     * @brief 打印抽象语法树。
     * @param[in] output_stream 输出流。
     * @return 打印操作的结果。
     * @date 2024-10-27
     */
    Result print(std::stringstream &output_stream) const override;
};

//////////////////////////////////////////
// 表达式
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
 * @brief 基本表达式抽象语法树类。
 * @date 2024-10-27
 */
class PrimaryExpAST : public BaseAST
{
public:
    std::optional<std::unique_ptr<BaseAST>> exp; // 可选的表达式
    std::optional<int> number;                   // 可选的数字

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
