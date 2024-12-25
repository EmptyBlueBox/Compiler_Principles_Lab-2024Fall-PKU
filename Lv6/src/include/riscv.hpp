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

#include "koopa.h"
#include "riscv_util.hpp"

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
 * @brief 访问 RISC-V 汇编代码的一个 load 指令
 * @param[in] load 内存中的 RISC-V 汇编代码 load 指令
 * @author Yutong Liang
 * @date 2024-12-23
 */
void visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);

/**
 * @brief 访问 RISC-V 汇编代码的一个 store 指令
 * @param[in] store 内存中的 RISC-V 汇编代码 store 指令
 * @param[in] value 这个 store 指令本身的 value, 这是一个 **指针**, 用于找寻这个 value 存在哪个寄存器里面了
 * @author Yutong Liang
 * @date 2024-12-23
 */
void visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value);

/**
 * @brief 访问 RISC-V 汇编代码的一个返回指令
 * @param[in] ret 内存中的 RISC-V 汇编代码返回指令本身, 带有 koopa_raw_value_t 类型的返回值
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_return_t &ret);

/**
 * @brief 访问 RISC-V 汇编代码的一条双目运算指令
 * @param[in] binary 双目运算指令
 * @param[in] value 这个双目运算指令本身的 value, 这是一个 **指针**, 用于找寻这个 value 存在哪个寄存器里面了
 * @author Yutong Liang
 * @date 2024-11-13
 */
void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);
