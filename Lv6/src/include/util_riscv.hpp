#pragma once

#include <string>
#include <unordered_map>

#include "koopa.h"

/**
 * @brief 单个函数的栈管理器, 是一个函数使用的, 可以维护值 (比如 `@x`, `%1`) 和栈地址的关系
 * @author Yutong Liang
 * @date 2024-12-22
 */
class StackManager
{
private:
    // 栈帧大小, 初始化的时候确定的, 单位是字节
    int stack_size;

    // 栈帧当前使用情况, 初始化时0, 直到增长为 stack_size 为止, 单位是字节
    int stack_used_byte;

    // 值到栈地址的映射, 栈地址的表示方法是 "sp + offset" 中的 int offset
    std::unordered_map<koopa_raw_value_t, int> value_to_stack_offset;

public:
    // 构造函数
    StackManager() : stack_size(0), stack_used_byte(0) {}
    StackManager(int stack_size) : stack_size(stack_size), stack_used_byte(0) {}

    // 保存一个值到栈中, 然后栈帧使用情况自动增加 4 字节
    void save_value_to_stack(const koopa_raw_value_t &value);

    // 获取栈帧使用情况
    int get_stack_used_byte() const;

    // 获取栈帧大小
    int get_num_stack_frame_byte() const;

    // 获取某个值对应的栈地址
    int get_value_stack_offset(const koopa_raw_value_t &value);
};

/**
 * @brief 寄存器和所有函数的栈管理器, 是全局共用的, 可以维护值和寄存器的关系, 可以维护一个值和这个值对应的函数的栈信息
 * @note 寄存器分配原则: 每一条 koopa 指令使用自己的寄存器然后释放自己的寄存器, 详细证明和说明如下
 * @note 因为每一行 koopa 代码只会使用 `@x`, `%1`, `1` 这样的值, 而这些值在 RISC-V 中要么在内存中, 要么就是立即数, 所以任意两个 koopa 指令之间是不会产生寄存器复用的
 * @note 因此我们在后端访问每一个 koopa_raw_value_t 类型的时候, 注意只访问 koopa 指令, 这样两个 visit(koopa_raw_value_t) 之间就不会产生寄存器复用了
 * @author Yutong Liang
 * @date 2024-11-28
 */
class ContextManager
{
private:
    // 值到寄存器名称的映射
    std::unordered_map<koopa_raw_value_t, std::string> _value_to_reg_string;

    // 存储当前所有寄存器是否有不能被覆盖的值
    std::unordered_map<std::string, bool> _reg_is_used;

    /**
     * @brief 设置一个值对应哪个寄存器, 内部函数不被外部调用
     * @param[in] value 值
     * @param[in] reg_string 寄存器名称
     * @author Yutong Liang
     * @date 2024-11-28
     */
    void _set_value_to_reg_string(const koopa_raw_value_t &value, const std::string &reg_string);

    // 函数名到这个函数的 StackManager 的映射
    std::unordered_map<std::string, StackManager> _function_name_to_stack_manager;

    // 当前正在处理的函数的函数名
    std::string current_function_name;

public:
    /**
     * @brief 构造函数, 初始化所有寄存器为未占用
     * @author Yutong Liang
     * @date 2024-11-29
     */
    ContextManager()
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
     * @brief 获取一个新的寄存器, 但是立刻就被释放, 只是用作临时中转
     * @return 寄存器名称
     * @author Yutong Liang
     * @date 2024-12-22
     */
    std::string new_temp_reg();

    /**
     * @brief 找出这个值占用哪个寄存器, 用于输出 RISC-V 汇编代码
     * @param[in] value 值
     * @return 寄存器名称
     * @author Yutong Liang
     * @date 2024-11-28
     */
    std::string value_to_reg_string(const koopa_raw_value_t &value);

    /**
     * @brief 获取当前正在处理的函数的栈管理器
     * @return 当前正在处理的函数的栈管理器
     * @author Yutong Liang
     * @date 2024-12-22
     */
    StackManager &get_current_function_stack_manager();

    /**
     * @brief 初始化一个函数的栈管理器
     * @param[in] function_name 函数名
     * @param[in] stack_size 栈帧大小
     * @author Yutong Liang
     * @date 2024-12-22
     */
    void init_stack_manager_for_one_function(const std::string &function_name, int stack_size);
};

/**
 * @brief RISC-V 汇编打印器, 可以帮助打印 RISC-V 汇编代码
 * @author Yutong Liang
 * @date 2024-11-28
 */
class RISCVPrinter
{
public:
    void ret();
    void seqz(const std::string &rd, const std::string &rs1);
    void snez(const std::string &rd, const std::string &rs1);
    void or_(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void and_(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void xor_(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void add(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void addi(const std::string &rd, const std::string &rs1, const int &imm);
    void sub(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void mul(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void div(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void rem(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void sgt(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void slt(const std::string &rd, const std::string &rs1, const std::string &rs2);
    void li(const std::string &rd, const int &imm);
    void mv(const std::string &rd, const std::string &rs1);
    void lw(const std::string &rd, const std::string &base, const int &bias, ContextManager &context_manager);
    void sw(const std::string &rs1, const std::string &base, const int &bias, ContextManager &context_manager);
    void add_sp(const int &bias, ContextManager &context_manager);
};
