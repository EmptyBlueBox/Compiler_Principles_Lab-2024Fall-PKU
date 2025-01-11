#include <iostream>

#include "include/riscv_util.hpp"

////////////////////////////////////////////////////
// StackManager
////////////////////////////////////////////////////

void StackManager::save_value_to_stack(const koopa_raw_value_t &value)
{
    // 如果 value 不在 value_to_stack_offset 中, 则需要分配新的空间
    if (value_to_stack_offset.find(value) == value_to_stack_offset.end())
    {
        value_to_stack_offset[value] = stack_used_byte;
        stack_used_byte += 4;
        if (stack_used_byte > stack_size)
        {
            throw std::runtime_error("save_value_to_stack: stack overflow");
        }
    }
    else
    {
        // 如果 value 在 value_to_stack_offset 中, 则不用分配新的空间, 什么都不用干
    }
}

int StackManager::get_stack_used_byte() const
{
    return stack_used_byte;
}

int StackManager::get_num_stack_frame_byte() const
{
    return stack_size;
}

int StackManager::get_value_stack_offset(const koopa_raw_value_t &value)
{
    if (value_to_stack_offset.find(value) == value_to_stack_offset.end())
    {
        throw std::runtime_error("get_value_stack_offset: value not found in this stack frame");
    }
    return value_to_stack_offset[value];
}

////////////////////////////////////////////////////
// RISCVContextManager
////////////////////////////////////////////////////

void RISCVContextManager::init_global_var(const koopa_raw_value_t &value)
{
    _value_to_global_var_index[value] = global_var_index;
    global_var_index++;
}

std::string RISCVContextManager::get_global_var_name(const koopa_raw_value_t &value)
{
    if (_value_to_global_var_index.find(value) == _value_to_global_var_index.end())
    {
        throw std::runtime_error("get_global_var_name: global variable not found");
    }
    return "global_var_" + std::to_string(_value_to_global_var_index[value]);
}

void RISCVContextManager::set_reg_free(const koopa_raw_value_t &value)
{
    if (_value_to_reg_string.find(value) == _value_to_reg_string.end())
    {
        throw std::runtime_error("set_reg_free: value not found in this stack frame");
    }
    _reg_is_used[value_to_reg_string(value)] = false;
    _value_to_reg_string.erase(value);
}

bool RISCVContextManager::exist(const koopa_raw_value_t &value)
{
    return _value_to_reg_string.find(value) != _value_to_reg_string.end();
}

void RISCVContextManager::allocate_reg(const koopa_raw_value_t &value, bool is_zero)
{
    if ((_value_to_reg_string.find(value) != _value_to_reg_string.end()) && _reg_is_used[value_to_reg_string(value)] == true)
    {
        throw std::runtime_error("allocate_reg: value already allocated, the value kind is " + std::to_string(value->kind.tag) + "\n0: Integer, 8: Load, 9: Store, 12: Binary, 13: Branch, 14: Jump, 15: Call, 16: Return");
    }
    if (is_zero)
    {
        _set_value_to_reg_string(value, "x0");
        return;
    }
    else
    {
        // 选择一个未被占用的寄存器
        for (int i = 0; i <= 6; ++i)
        {
            if (!_reg_is_used["t" + std::to_string(i)])
            {
                _set_value_to_reg_string(value, "t" + std::to_string(i));
                _reg_is_used["t" + std::to_string(i)] = true;
                return;
            }
        }
        for (int i = 0; i <= 7; ++i)
        {
            if (!_reg_is_used["a" + std::to_string(i)])
            {
                _set_value_to_reg_string(value, "a" + std::to_string(i));
                _reg_is_used["a" + std::to_string(i)] = true;
                return;
            }
        }
    }
}

std::string RISCVContextManager::new_temp_reg()
{
    // 选择一个未被占用的寄存器
    for (int i = 0; i <= 6; ++i)
    {
        if (!_reg_is_used["t" + std::to_string(i)])
        {
            std::string reg_name = "t" + std::to_string(i);
            return reg_name;
        }
    }
    for (int i = 0; i <= 7; ++i)
    {
        if (!_reg_is_used["a" + std::to_string(i)])
        {
            std::string reg_name = "a" + std::to_string(i);
            return reg_name;
        }
    }
    throw std::runtime_error("new_temp_reg: no free register found");
}

std::string RISCVContextManager::value_to_reg_string(const koopa_raw_value_t &value)
{
    if (_value_to_reg_string.find(value) == _value_to_reg_string.end())
    {
        throw std::runtime_error("value_to_reg_string: value not found");
    }
    return _value_to_reg_string[value];
}

void RISCVContextManager::_set_value_to_reg_string(const koopa_raw_value_t &value, const std::string &reg_string)
{
    if (_value_to_reg_string.find(value) != _value_to_reg_string.end())
    {
        throw std::runtime_error("_set_value_to_reg_string: value already exists in the map, the value kind is " + std::to_string(value->kind.tag) + "\n0: Integer, 12: Binary");
    }
    _value_to_reg_string[value] = reg_string;
}

StackManager &RISCVContextManager::get_current_function_stack_manager()
{
    if (_function_name_to_stack_manager.find(current_function_name) == _function_name_to_stack_manager.end())
    {
        throw std::runtime_error("get_current_function_stack_manager: function not found");
    }
    return _function_name_to_stack_manager[current_function_name];
}

void RISCVContextManager::init_stack_manager_for_one_function(const std::string &function_name, int stack_size, int num_args_on_stack)
{
    if (_function_name_to_stack_manager.find(function_name) != _function_name_to_stack_manager.end())
    {
        throw std::runtime_error("init_stack_manager_for_one_function: function already exists");
    }
    _function_name_to_stack_manager[function_name] = StackManager(stack_size, num_args_on_stack);
    current_function_name = function_name;
}

////////////////////////////////////////////////////
// RISCV 语句
////////////////////////////////////////////////////

void RISCVPrinter::data()
{
    std::cout << "\n\t.data" << std::endl;
}

void RISCVPrinter::text()
{
    std::cout << "\n\t.text" << std::endl;
}

void RISCVPrinter::globl(const std::string &name)
{
    std::cout << "\t.globl " << name << std::endl;
}

void RISCVPrinter::word(const int &value)
{
    std::cout << "\t.word " << value << std::endl;
}

void RISCVPrinter::zero(const int &len)
{
    std::cout << "\t.zero " << len << std::endl;
}

void RISCVPrinter::label(const std::string &name)
{
    std::cout << name << ":" << std::endl;
}

////////////////////////////////////////////////////
// 调用和返回
////////////////////////////////////////////////////

void RISCVPrinter::call(const std::string &func_name)
{
    std::cout << "\tcall " << func_name << std::endl;
}

void RISCVPrinter::ret()
{
    std::cout << "\tret" << std::endl;
}

////////////////////////////////////////////////////
// 单目运算
////////////////////////////////////////////////////

void RISCVPrinter::seqz(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tseqz " << rd << ", " << rs1 << std::endl;
}

void RISCVPrinter::snez(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tsnez " << rd << ", " << rs1 << std::endl;
}

////////////////////////////////////////////////////
// 双目运算
////////////////////////////////////////////////////

void RISCVPrinter::or_(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tor " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::and_(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tand " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::xor_(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\txor " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::add(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tadd " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::addi(const std::string &rd, const std::string &rs1, const int &imm, RISCVContextManager &context_manager)
{
    if (imm >= -2048 && imm < 2048)
    {
        std::cout << "\taddi " << rd << ", " << rs1 << ", " << imm << std::endl;
    }
    else
    {
        std::string reg = context_manager.new_temp_reg();
        li(reg, imm);
        std::cout << "\tadd " << rd << ", " << rs1 << ", " << reg << std::endl;
    }
}

void RISCVPrinter::sub(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tsub " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::mul(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tmul " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::div(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tdiv " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::rem(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\trem " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::sgt(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tsgt " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

void RISCVPrinter::slt(const std::string &rd, const std::string &rs1, const std::string &rs2)
{
    std::cout << "\tslt " << rd << ", " << rs1 << ", " << rs2 << std::endl;
}

////////////////////////////////////////////////////
// 移动和访存
////////////////////////////////////////////////////

void RISCVPrinter::li(const std::string &rd, const int &imm)
{
    std::cout << "\tli " << rd << ", " << imm << std::endl;
}

void RISCVPrinter::mv(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tmv " << rd << ", " << rs1 << std::endl;
}

void RISCVPrinter::la(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tla " << rd << ", " << rs1 << std::endl;
}

void RISCVPrinter::lw(const std::string &rd, const std::string &base, const int &bias, RISCVContextManager &context_manager)
{
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048)
    {
        std::cout << "\tlw " << rd << ", " << bias << "(" << base << ")" << std::endl;
    }
    else
    {
        std::string reg = context_manager.new_temp_reg();
        li(reg, bias);
        add(reg, reg, base);
        std::cout << "\tlw " << rd << ", " << "(" << reg << ")" << std::endl;
    }
}

void RISCVPrinter::sw(const std::string &rs1, const std::string &base, const int &bias, RISCVContextManager &context_manager)
{
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048)
    {
        std::cout << "\tsw " << rs1 << ", " << bias << "(" << base << ")" << std::endl;
    }
    else
    {
        std::string reg = context_manager.new_temp_reg();
        li(reg, bias);
        add(reg, reg, base);
        std::cout << "\tsw " << rs1 << ", " << "(" << reg << ")" << std::endl;
    }
}

////////////////////////////////////////////////////
// 分支
////////////////////////////////////////////////////

void RISCVPrinter::bnez(const std::string &cond, const std::string &label)
{
    std::cout << "\tbnez " << cond << ", " << label << std::endl;
}

void RISCVPrinter::beqz(const std::string &cond, const std::string &label)
{
    std::cout << "\tbeqz " << cond << ", " << label << std::endl;
}

void RISCVPrinter::jump(const std::string &label)
{
    std::cout << "\tj " << label << std::endl;
}
