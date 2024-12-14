/**
 * @file include/backend.hpp
 * @brief 定义了后端函数
 * @author Yutong Liang
 * @date 2024-11-13
 */

#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include "koopa.h"

/**
 * @brief 后端函数, 使用 koopa.h 将 Koopa IR 转换为内存中的 RISC-V 汇编代码, 然后 DFS 遍历 RISC-V 汇编代码, 将其输出到内存中
 * @param[in] koopa_str 输入的 Koopa IR 字符串
 * @return 0 表示成功, 其他值表示失败
 * @author Yutong Liang
 * @date 2024-11-13
 */
int backend(const char *koopa_str);

/**
 * @brief 进入每一个节点, 如果它包含很多同类型的东西, 比如一个函数有很多的基本块, 一个基本块有很多指令,
 * 那么这一堆基本块或者指令就会存在一个 raw slice 类型中, 所以只需要访问一次 raw slice 即可,
 * raw slice 会把这一堆东西逐个帮你访问
 * @param[in] slice 多个相同类型的元素的集合
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_slice_t &slice);

/**
 * @brief 访问 RISC-V 汇编代码所有程序
 * @param[in] program 内存中的 RISC-V 汇编代码程序
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_program_t &program);

/**
 * @brief 访问 RISC-V 汇编代码的一个函数
 * @param[in] func 内存中的 RISC-V 汇编代码函数
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_function_t &func);

/**
 * @brief 访问 RISC-V 汇编代码的一个基本块
 * @param[in] bb 内存中的 RISC-V 汇编代码基本块
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_basic_block_t &bb);

/**
 * @brief 访问 RISC-V 汇编代码的一条指令或者一个值
 * @param[in] value 内存中的 RISC-V 汇编代码指令或者值
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_value_t &value);

/**
 * @brief 访问 RISC-V 汇编代码的一个返回指令
 * @param[in] ret 内存中的 RISC-V 汇编代码返回指令本身, 带有 koopa_raw_value_t 类型的返回值
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_return_t &ret);

/**
 * @brief 访问 RISC-V 汇编代码的一个整数
 * @param[in] integer 内存中的 RISC-V 汇编代码整数
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t &value);

/**
 * @brief 访问 RISC-V 汇编代码的一条双目运算指令
 * @param[in] binary 双目运算指令
 * @param[in] value 这个双目运算指令本身的 value, 这是一个 **指针**, 用于找寻这个 value 存在哪个寄存器里面了
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

/**
 * @brief 寄存器管理器, 可以请求一个寄存器, 存储一个值对应哪个寄存器, 并判断一个值占用了哪个寄存器
 * @author Yutong Liang
 * @date 2024-11-28
 */
class RegisterManager
{
private:
    // 值到寄存器名称的映射
    std::unordered_map<koopa_raw_value_t, std::string> _value_to_reg_string;
    // 存储当前所有寄存器是否可能会再次被利用, 比如将立即数转移给 a0 寄存器, 我们现在就认为 a0 寄存器被占用了, 但是如果 a0 寄存器之后被调用了, 这个立即数被使用过了, 那么 a0 寄存器就会被标记为不被占用, 因为到目前为止我们认为每一个结果只被使用一次
    std::unordered_map<std::string, bool> _reg_is_used;
    /**
     * @brief 设置一个值对应哪个寄存器, 内部函数不被外部调用
     * @param[in] value 值
     * @param[in] reg_string 寄存器名称
     * @author Yutong Liang
     * @date 2024-11-28
     */
    void _set_value_to_reg_string(const koopa_raw_value_t &value, const std::string &reg_string);

public:
    /**
     * @brief 构造函数, 初始化所有寄存器为未占用
     * @author Yutong Liang
     * @date 2024-11-29
     */
    RegisterManager()
    {
        // 初始化所有寄存器为未占用
        for (int i = 0; i <= 6; ++i)
        {
            _reg_is_used["t" + std::to_string(i)] = false;
        }
        for (int i = 0; i <= 7; ++i)
        {
            _reg_is_used["a" + std::to_string(i)] = false;
        }
    }

    /**
     * @brief 设置一个值对应的寄存器为未占用, 当一个值被使用过之后, 我们将它占用的寄存器设置为未占用, 因为我们认为每一个结果只被使用一次
     * @param[in] value 值
     * @author Yutong Liang
     * @date 2024-11-29
     */
    void set_reg_free(const koopa_raw_value_t &value);

    /**
     * @brief 判断一个值是否已经分配了寄存器
     * @param[in] value 值
     * @return 是否已经分配了寄存器
     * @author Yutong Liang
     * @date 2024-11-28
     */
    bool exist(const koopa_raw_value_t &value);

    /**
     * @brief 给一个值分配一个寄存器, 自动选择一个未被占用的寄存器
     * @note x0 是一个特殊的寄存器, 它的值恒为 0, 且向它写入的任何数据都会被丢弃, t0 到 t6 寄存器, 以及 a0 到 a7 寄存器可以用来存放临时值
     * @param[in] value 值
     * @param[in] is_zero 如果是立即数, 那么是否是立即数 0
     * @author Yutong Liang
     * @date 2024-11-29
     */
    void allocate_reg(const koopa_raw_value_t &value, bool is_zero = false);

    /**
     * @brief 找出这个值占用哪个寄存器, 用于输出 RISC-V 汇编代码
     * @param[in] value 值
     * @return 寄存器名称
     * @author Yutong Liang
     * @date 2024-11-28
     */
    std::string value_to_reg_string(const koopa_raw_value_t &value);
};
