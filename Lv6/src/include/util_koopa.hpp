/**
 * @brief 定义一些通用的类, 比如符号表等
 * @author Yutong Liang
 * @date 2024-11-13
 */

#pragma once

#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
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
     * @brief 当前计算值存储在 `%current_value_symbol_index` 这个寄存器中
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
 * @brief 符号
 * @date 2024-12-22
 */
class Symbol
{
public:
    enum class Type
    {
        VAR,
        VAL
    };
    Type type;
    int val; // 如果 type 是 VAL, 那么 val 是立即数的数值; 如果 type 是 VAR, 那么 val 是变量的层级, 比如 `a = 2;` 如果在符号表中在层级 1 找到这个符号, 那么就会返回 1, 得到 @a_1
    Symbol() : type(Type::VAL), val(0) {}
    Symbol(Type type, int val) : type(type), val(val) {}
};

/**
 * @brief 符号表
 * @date 2024-12-22
 */
class SymbolTable
{
private:
    std::vector<std::unordered_map<std::string, Symbol>> symbol_table; // 每进入一个块, 就创建一个新的符号表, 块包括函数的大括号和语句块的大括号
    bool is_returned = false;

public:
    void new_symbol_table_hierarchy();
    void delete_symbol_table_hierarchy();
    void insert_symbol(const std::string &name, Symbol symbol);
    Symbol read(const std::string &name);
    void set_returned(bool is_returned);
    bool get_returned();
};
