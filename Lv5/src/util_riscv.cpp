#include <iostream>

#include "include/util_riscv.hpp"

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

void ContextManager::set_reg_free(const koopa_raw_value_t &value)
{
    if (_value_to_reg_string.find(value) == _value_to_reg_string.end())
    {
        throw std::runtime_error("set_reg_free: value not found in this stack frame");
    }
    _reg_is_used[value_to_reg_string(value)] = false;
    _value_to_reg_string.erase(value);
}

bool ContextManager::exist(const koopa_raw_value_t &value)
{
    return _value_to_reg_string.find(value) != _value_to_reg_string.end();
}

void ContextManager::allocate_reg(const koopa_raw_value_t &value, bool is_zero)
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

std::string ContextManager::new_temp_reg()
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

std::string ContextManager::value_to_reg_string(const koopa_raw_value_t &value)
{
    if (_value_to_reg_string.find(value) == _value_to_reg_string.end())
    {
        throw std::runtime_error("value_to_reg_string: value not found");
    }
    return _value_to_reg_string[value];
}

void ContextManager::_set_value_to_reg_string(const koopa_raw_value_t &value, const std::string &reg_string)
{
    if (_value_to_reg_string.find(value) != _value_to_reg_string.end())
    {
        throw std::runtime_error("_set_value_to_reg_string: value already exists in the map, the value kind is " + std::to_string(value->kind.tag) + "\n0: Integer, 12: Binary");
    }
    _value_to_reg_string[value] = reg_string;
}

StackManager &ContextManager::get_current_function_stack_manager()
{
    if (_function_name_to_stack_manager.find(current_function_name) == _function_name_to_stack_manager.end())
    {
        throw std::runtime_error("get_current_function_stack_manager: function not found");
    }
    return _function_name_to_stack_manager[current_function_name];
}

void ContextManager::init_stack_manager_for_one_function(const std::string &function_name, int stack_size)
{
    if (_function_name_to_stack_manager.find(function_name) != _function_name_to_stack_manager.end())
    {
        throw std::runtime_error("init_stack_manager_for_one_function: function already exists");
    }
    _function_name_to_stack_manager[function_name] = StackManager(stack_size);
    current_function_name = function_name;
}

void RISCVPrinter::ret()
{
    std::cout << "\tret" << std::endl;
}

void RISCVPrinter::seqz(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tseqz " << rd << ", " << rs1 << std::endl;
}

void RISCVPrinter::snez(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tsnez " << rd << ", " << rs1 << std::endl;
}

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

void RISCVPrinter::addi(const std::string &rd, const std::string &rs1, const int &imm)
{
    std::cout << "\taddi " << rd << ", " << rs1 << ", " << imm << std::endl;
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

void RISCVPrinter::li(const std::string &rd, const int &imm)
{
    std::cout << "\tli " << rd << ", " << imm << std::endl;
}

void RISCVPrinter::mv(const std::string &rd, const std::string &rs1)
{
    std::cout << "\tmv " << rd << ", " << rs1 << std::endl;
}

void RISCVPrinter::lw(const std::string &rd, const std::string &base, const int &bias, ContextManager &context_manager)
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

void RISCVPrinter::sw(const std::string &rs1, const std::string &base, const int &bias, ContextManager &context_manager)
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
