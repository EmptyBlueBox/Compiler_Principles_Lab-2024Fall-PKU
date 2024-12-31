/**
 * @brief 定义一些通用的类, 比如符号表等
 * @author Yutong Liang
 * @date 2024-11-13
 */

#pragma once

#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <utility>
#include <stack>
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
    Type type;                                   // 结果的类型
    int val;                                     // 结果的值
    bool control_flow_returned = false;          // 控制流是否已经返回, 比如普通的 return 语句, 或者 if ... else ... 两个都 return 了就返回 true
    bool control_flow_while_interrupted = false; // while 中的控制流是否被打断

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
 * @brief 上下文管理器, 用于管理符号表和 If ... Else ... 语句的数量, 从而输出恰当的 %then_1: 和 %else_1:
 * @date 2024-12-22
 */
class KoopaContextManager
{
private:
    // 每进入一个块, 就创建一个新的符号表, 块包括函数的大括号和语句块的大括号
    std::vector<std::unordered_map<std::string, Symbol>> symbol_tables;
    // 用于判断当前符号是否在当前下标被分配, 比如 @a_1 在 symbol_tables[0] 中被分配, 那么 is_symbol_allocated_in_this_level[std::make_pair("a", 1)] == true
    // 用来避免如下的例子中 a 被分配了两次
    // int a = 1;
    // if (a) {
    //     {int a = 2;}
    //     {int a = 3;}
    // }
    // 这样的重复分配 @a_2 是错误的
    // @a_2 = alloc i32
    // store 2, @a_2
    // @a_2 = alloc i32
    // store 3, @a_2
    std::map<std::pair<std::string, int>, bool> _is_symbol_allocated_in_this_level;

public:
    // 当前的 if ... else ... 语句数量, 遇见一个加一
    int total_if_else_statement_count = 0;

    // 当前的 while ... 语句数量, 遇见一个加一
    int total_while_statement_count = 0;

    // 当前的 while ... 语句的栈, 用于管理 while ... 语句的嵌套之后如何判断 break 和 continue 语句应该跳转到哪个 while ... 语句的结束块
    std::stack<int> while_statement_stack;

    // 当前的 && 语句数量, 遇见一个加一
    int total_and_statement_count = 0;

    // 当前的 || 语句数量, 遇见一个加一
    int total_or_statement_count = 0;

    // 每进入一个大括号 (或者 if ... else ... 语句) 就创建一个新的符号表
    void new_symbol_table_hierarchy();

    // 离开大括号 (或者 if ... else ... 语句) 就删除一个符号表
    void delete_symbol_table_hierarchy();

    // 符号表操作, 在当前符号表中插入一个符号对应的字符串 (不包含后缀) 和 Symbol 对象 (立即数还是变量的层级)
    void insert_symbol(const std::string &name, Symbol symbol);

    // 在当前符号表中查找一个符号对应的 Symbol 对象, 从最内层的符号表开始查找, 逐渐向外层查找
    Symbol name_to_symbol(const std::string &name);

    // 设置一个符号在当前层级的符号表中被分配
    void set_symbol_allocated_in_this_level(const std::string &name);

    // 判断一个符号在当前层级的符号表中是否被分配
    bool is_symbol_allocated_in_this_level(const std::string &name);
};
