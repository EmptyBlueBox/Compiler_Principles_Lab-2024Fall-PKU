#include "include/backend.hpp"

RegisterManager register_manager;

int backend(const char *koopa_str)
{
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(koopa_str, &program);
    assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);

    return 0;
}

// 进入每一个节点, 如果它包含很多同类型的东西, 比如一个函数有很多的基本块, 一个基本块有很多指令,
// 那么这一堆基本块或者指令就会存在一个 raw slice 类型中, 所以只需要访问一次 raw slice 即可,
// raw slice 会把这一堆东西逐个帮你访问
void visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问 raw program
void visit(const koopa_raw_program_t &program)
{
    // 访问所有全局变量
    visit(program.values);

    // Print ".text"
    std::cout << "\t.text" << std::endl;

    // 访问所有函数
    visit(program.funcs);
}

// 访问函数
void visit(const koopa_raw_function_t &func)
{
    // 访问所有基本块
    std::cout << "\t.globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;
    visit(func->bbs);
}

// 访问基本块
void visit(const koopa_raw_basic_block_t &bb)
{
    // 访问所有指令
    visit(bb->insts);
}

// 访问指令
void visit(const koopa_raw_value_t &value)
{
    // std::cerr << "visit value" << std::endl;
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        visit(kind.data.integer, value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        visit(kind.data.binary, value);
        break;
    default:
        // 其他类型暂时遇不到
        throw std::runtime_error("visit: invalid instruction");
    }
}

// 访问 return 指令
void visit(const koopa_raw_return_t &ret)
{
    // std::cerr << "visit return" << std::endl;
    // 根据 ret 的 value 类型判断后续需要如何访问
    if (ret.value)
    {
        // 特判如果是立即数, 则直接赋值给 a0 寄存器, 跳过访问 value 的过程
        if (ret.value->kind.tag == KOOPA_RVT_INTEGER)
        {
            std::cout << "\tli a0, " << ret.value->kind.data.integer.value << std::endl;
        }
        // 否则, 访问这个值, 然后把这个值存储在的寄存器名称移动给 a0 寄存器, 注意不是 li
        else
        {
            bool is_allocated = register_manager.exist(ret.value);
            if (!is_allocated)
            {
                visit(ret.value);
            }
            std::cout << "\tmv a0, " << register_manager.value_to_reg_string(ret.value) << std::endl;
        }
    }
    // 如果 ret 的 value 为空, 则直接赋值 0 给 a0 寄存器, 然后返回
    else
    {
        std::cout << "\tli a0, 0" << std::endl;
    }
    std::cout << "\tret" << std::endl;
}

// 访问 integer
void visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t &value)
{
    // std::cerr << "visit integer" << std::endl;
    if (integer.value == 0)
    {
        register_manager.allocate_reg(value, true);
    }
    else
    {
        register_manager.allocate_reg(value);
        std::cout << "\tli " << register_manager.value_to_reg_string(value) << ", " << integer.value << std::endl;
    }
}

// 访问 binary 指令
void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    // std::cerr << "visit binary" << std::endl;
    // lhs 和 rhs 是否已经分配过寄存器, 如果没分配过, 则需要先访问 lhs 和 rhs, 访问过程中会分配寄存器, 注意 ricsv 不能直接操作立即数, 必须先加载到寄存器中!
    bool lhs_is_allocated = register_manager.exist(binary.lhs);
    if (!lhs_is_allocated)
    {
        visit(binary.lhs);
    }
    bool rhs_is_allocated = register_manager.exist(binary.rhs);
    if (!rhs_is_allocated)
    {
        visit(binary.rhs);
    }
    // 我们认为每个结果仅使用一次, 所以可以设置两个子结果的寄存器可以被覆盖了.
    // 比如将立即数转移给 a0 寄存器, 我们现在就认为 a0 寄存器被占用了, 但是如果 a0 寄存器之后被调用了, 这个立即数被使用过了, 那么 a0 寄存器就会被标记为不被占用, 因为到目前为止我们认为每一个结果只被使用一次
    register_manager.set_reg_free(binary.lhs);
    register_manager.set_reg_free(binary.rhs);
    register_manager.allocate_reg(value);

    // 获取当前结果, lhs 和 rhs 对应的寄存器名称
    std::string cur = register_manager.value_to_reg_string(value);
    std::string lhs = register_manager.value_to_reg_string(binary.lhs);
    std::string rhs = register_manager.value_to_reg_string(binary.rhs);

    // 根据二元运算符的类型进行处理
    switch (binary.op)
    {
    case KOOPA_RBO_EQ:
        std::cout << "\txor " << cur << ", " << lhs << ", " << rhs << std::endl;
        std::cout << "\tseqz " << cur << ", " << cur << std::endl;
        break;
    case KOOPA_RBO_NOT_EQ:
        std::cout << "\txor " << cur << ", " << lhs << ", " << rhs << std::endl;
        std::cout << "\tsnez " << cur << ", " << cur << std::endl;
        break;
    case KOOPA_RBO_GT:
        std::cout << "\tsgt " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_LT:
        std::cout << "\tslt " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_GE:
        std::cout << "\tslt " << cur << ", " << lhs << ", " << rhs << std::endl;
        std::cout << "\tseqz " << cur << ", " << cur << std::endl;
        break;
    case KOOPA_RBO_LE:
        std::cout << "\tsgt " << cur << ", " << lhs << ", " << rhs << std::endl;
        std::cout << "\tseqz " << cur << ", " << cur << std::endl;
        break;
    case KOOPA_RBO_ADD:
        std::cout << "\tadd " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_SUB:
        std::cout << "\tsub " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_MUL:
        std::cout << "\tmul " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_DIV:
        std::cout << "\tdiv " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_MOD:
        std::cout << "\trem " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_AND:
        std::cout << "\tand " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    case KOOPA_RBO_OR:
        std::cout << "\tor " << cur << ", " << lhs << ", " << rhs << std::endl;
        break;
    default:
        throw std::runtime_error("visit: invalid binary operator");
    }
}

void RegisterManager::set_reg_free(const koopa_raw_value_t &value)
{
    _reg_is_used[value_to_reg_string(value)] = false;
}

bool RegisterManager::exist(const koopa_raw_value_t &value)
{
    return _value_to_reg_string.find(value) != _value_to_reg_string.end();
}

void RegisterManager::allocate_reg(const koopa_raw_value_t &value, bool is_zero)
{
    if (_value_to_reg_string.find(value) != _value_to_reg_string.end())
    {
        throw std::runtime_error("allocate_reg: value already allocated, the value kind is " + std::to_string(value->kind.tag) + "\n0: Integer, 12: Binary");
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

std::string RegisterManager::value_to_reg_string(const koopa_raw_value_t &value)
{
    if (_value_to_reg_string.find(value) == _value_to_reg_string.end())
    {
        throw std::runtime_error("value_to_reg_string: value not found");
    }
    return _value_to_reg_string[value];
}

void RegisterManager::_set_value_to_reg_string(const koopa_raw_value_t &value, const std::string &reg_string)
{
    if (_value_to_reg_string.find(value) != _value_to_reg_string.end())
    {
        throw std::runtime_error("_set_value_to_reg_string: value already exists in the map, the value kind is " + std::to_string(value->kind.tag) + "\n0: Integer, 12: Binary");
    }
    _value_to_reg_string[value] = reg_string;
}
