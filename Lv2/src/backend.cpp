#include "include/backend.hpp"

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
        visit(kind.data.integer);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

void visit(const koopa_raw_return_t &ret)
{
    // 根据 ret 的 value 类型判断后续需要如何访问
    if (ret.value)
    {
        std::cout << "\tli a0, ";
        visit(ret.value);
        std::cout << std::endl;
    }
    else
    {
        std::cout << "\tli a0, 0" << std::endl;
    }
    std::cout << "\tret" << std::endl;
}

void visit(const koopa_raw_integer_t &integer)
{
    std::cout << integer.value;
}
