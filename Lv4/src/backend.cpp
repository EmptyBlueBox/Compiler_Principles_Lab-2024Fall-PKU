#include "include/backend.hpp"
#include "include/util_riscv.hpp"

// 所有代码共用的寄存器和栈管理器
ContextManager context_manager;

// 所有代码共用的 RISC-V 汇编打印器
RISCVPrinter riscv_printer;

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

// 进入每一个节点, 如果它包含很多同类型的东西, 比如一个函数有很多的基本块, 一个基本块有很多指令, 那么这一堆基本块或者指令就会存在一个 raw slice 类型中, 所以只需要访问一次 raw slice 即可, raw slice 会把这一堆东西逐个帮你访问
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
    // 获得函数名
    std::string function_name = func->name + 1;

    // 输出 RISC-V 的 prologue
    std::cout << "\t.globl " << function_name << std::endl;
    std::cout << function_name << ":" << std::endl;

    // 计算栈帧大小
    // 如何判断一个指令存在返回值呢 ? 你也许还记得 Koopa IR 是强类型 IR, 所有指令都是有类型的.如果指令的类型为 unit(类似 C / C++ 中的 void), 则这条指令不存在返回值. 在 C / C++ 中, 每个 koopa_raw_value_t 都有一个名叫 ty 的字段, 它的类型是 koopa_raw_type_t.koopa_raw_type_t 中有一个字段叫做 tag, 存储了这个类型具体是何种类型.如果它的值为 KOOPA_RTT_UNIT, 说明这个类型是 unit 类型.或者你实在懒得判断的话,给所有指令都分配栈空间也不是不行, 只不过这样会浪费一些栈空间.
    int num_stack_frame_byte = 0;
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        num_stack_frame_byte += bb->insts.len;
        for (size_t j = 0; j < bb->insts.len; ++j)
        {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag == KOOPA_RTT_UNIT)
            {
                num_stack_frame_byte -= 1;
            }
        }
    }

    // 计算栈帧大小
    num_stack_frame_byte *= 4;
    num_stack_frame_byte = (num_stack_frame_byte + 15) / 16 * 16; // 对齐到 16 的倍数

    // 初始化栈管理器
    context_manager.init_stack_manager_for_one_function(function_name, num_stack_frame_byte);

    // 继续输出 RISC-V 的 prologue
    riscv_printer.addi("sp", "sp", -num_stack_frame_byte);

    // 访问所有基本块
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
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令, 分配内存是实际存在的指令, 但是汇编语言的内存分配是直接用栈指针管理的, 所以不需要显式的内存分配, 可以忽略 koopa 的 alloc 指令
        // 比如 @x = alloc i32 这样一个指令, 如果只有这个指令本身, 并不需要做任何操作也能保证 RISC-V 的正确性
        // 只有在使用了 @x 的时候, 比如 store 10, @x 这样的指令, 才需要设定 @x 的栈地址, 所以 alloc 指令可以忽略
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        visit(kind.data.store, value);
        break;
    default:
        // 其他类型暂时遇不到
        throw std::runtime_error("visit: invalid instruction");
    }
}

// 访问 load 指令, load 的输入和输出都必然是内存
void visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value)
{
    // 当前函数的 StackManager
    StackManager &stack_manager = context_manager.get_current_function_stack_manager();
    // 给中间结果分配一个寄存器, 这个寄存器对应 value, 为什么不用临时寄存器? 因为 lw 和 sw 也可能使用寄存器, 可能造成冲突
    context_manager.allocate_reg(value);
    std::string temp_reg_name = context_manager.value_to_reg_string(value);
    // 从栈中加载数据到寄存器
    riscv_printer.lw(temp_reg_name, "sp", stack_manager.get_value_stack_offset(load.src), context_manager);
    // 将数据保存到栈中, 更新栈帧使用情况, 同时输出 sw 指令
    stack_manager.save_value_to_stack(value);
    riscv_printer.sw(temp_reg_name, "sp", stack_manager.get_value_stack_offset(value), context_manager);
    // 当前操作数所在的寄存器已经被使用过了, 释放
    context_manager.set_reg_free(value);
}

// 访问 store 指令, store 的输出必然是内存, 但是输入可能是内存或者立即数
void visit(const koopa_raw_store_t &store, const koopa_raw_value_t &value)
{
    // 当前函数的 StackManager
    StackManager &stack_manager = context_manager.get_current_function_stack_manager();
    // 给中间结果分配一个寄存器, 这个寄存器对应 value
    context_manager.allocate_reg(value);
    std::string temp_reg_name = context_manager.value_to_reg_string(value);
    // 判断 store.value 是否是立即数, 如果是立即数, 则需要先分配一个临时寄存器, 如果是内存, 则从内存中 lw 出来
    if (store.value->kind.tag == KOOPA_RVT_INTEGER)
    {
        riscv_printer.li(temp_reg_name, store.value->kind.data.integer.value);
    }
    else
    {
        riscv_printer.lw(temp_reg_name, "sp", stack_manager.get_value_stack_offset(store.value), context_manager);
    }
    // 保存 store.dest 到栈中
    stack_manager.save_value_to_stack(store.dest);
    riscv_printer.sw(temp_reg_name, "sp", stack_manager.get_value_stack_offset(store.dest), context_manager);
    // 当前操作数所在的寄存器已经被使用过了, 释放
    context_manager.set_reg_free(value);
}

// 访问 return 指令
void visit(const koopa_raw_return_t &ret)
{
    // 根据 ret 的 value 类型判断后续需要如何访问
    if (ret.value)
    {
        // 特判如果是立即数, 则直接赋值给 a0 寄存器, 跳过访问 value 的过程
        if (ret.value->kind.tag == KOOPA_RVT_INTEGER)
        {
            riscv_printer.li("a0", ret.value->kind.data.integer.value);
        }
        // 否则, 访问这个值, 然后把这个值存储在的内存移动给 a0 寄存器, 注意不是 li
        else
        {
            // 当前函数的 StackManager
            StackManager &stack_manager = context_manager.get_current_function_stack_manager();
            // 将 ret.value 的值从栈中加载到 a0 寄存器
            riscv_printer.lw("a0", "sp", stack_manager.get_value_stack_offset(ret.value), context_manager);
        }
    }
    // 如果 ret 的 value 为空, 则直接赋值 0 给 a0 寄存器, 然后返回
    else
    {
        riscv_printer.li("a0", 0);
    }
    // 恢复栈帧
    StackManager &stack_manager = context_manager.get_current_function_stack_manager();
    riscv_printer.addi("sp", "sp", stack_manager.get_num_stack_frame_byte());
    // 返回
    riscv_printer.ret();
}

// 访问 integer
void visit(const koopa_raw_integer_t &integer, const koopa_raw_value_t &value)
{
    if (integer.value == 0)
    {
        context_manager.allocate_reg(value, true);
    }
    else
    {
        context_manager.allocate_reg(value);
        riscv_printer.li(context_manager.value_to_reg_string(value), integer.value);
    }
}

// 访问 binary 指令
void visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    // 判断 lhs 是立即数还是内存, 如果是立即数就 li, 否则就 lw
    context_manager.allocate_reg(binary.lhs);
    std::string lhs = context_manager.value_to_reg_string(binary.lhs);
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        riscv_printer.li(lhs, binary.lhs->kind.data.integer.value);
    }
    else
    {
        // 当前函数的 StackManager
        StackManager &stack_manager = context_manager.get_current_function_stack_manager();
        // 从栈中加载数据到寄存器
        riscv_printer.lw(lhs, "sp", stack_manager.get_value_stack_offset(binary.lhs), context_manager);
    }
    // 判断 rhs 是立即数还是内存, 如果是立即数就 li, 否则就 lw
    context_manager.allocate_reg(binary.rhs);
    std::string rhs = context_manager.value_to_reg_string(binary.rhs);
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        riscv_printer.li(rhs, binary.rhs->kind.data.integer.value);
    }
    else
    {
        // 当前函数的 StackManager
        StackManager &stack_manager = context_manager.get_current_function_stack_manager();
        // 从栈中加载数据到寄存器
        riscv_printer.lw(rhs, "sp", stack_manager.get_value_stack_offset(binary.rhs), context_manager);
    }

    // 给结果分配一个寄存器, 分配之前可以先释放掉 lhs 和 rhs 对应的寄存器, 因为他们相当于已经加载进来了
    context_manager.set_reg_free(binary.lhs);
    context_manager.set_reg_free(binary.rhs);
    context_manager.allocate_reg(value);
    std::string cur = context_manager.value_to_reg_string(value);

    // 根据二元运算符的类型进行处理
    switch (binary.op)
    {
    case KOOPA_RBO_EQ:
        riscv_printer.xor_(cur, lhs, rhs);
        riscv_printer.seqz(cur, cur);
        break;
    case KOOPA_RBO_NOT_EQ:
        riscv_printer.xor_(cur, lhs, rhs);
        riscv_printer.snez(cur, cur);
        break;
    case KOOPA_RBO_GT:
        riscv_printer.sgt(cur, lhs, rhs);
        break;
    case KOOPA_RBO_LT:
        riscv_printer.slt(cur, lhs, rhs);
        break;
    case KOOPA_RBO_GE:
        riscv_printer.slt(cur, lhs, rhs);
        riscv_printer.seqz(cur, cur);
        break;
    case KOOPA_RBO_LE:
        riscv_printer.sgt(cur, lhs, rhs);
        riscv_printer.seqz(cur, cur);
        break;
    case KOOPA_RBO_ADD:
        riscv_printer.add(cur, lhs, rhs);
        break;
    case KOOPA_RBO_SUB:
        riscv_printer.sub(cur, lhs, rhs);
        break;
    case KOOPA_RBO_MUL:
        riscv_printer.mul(cur, lhs, rhs);
        break;
    case KOOPA_RBO_DIV:
        riscv_printer.div(cur, lhs, rhs);
        break;
    case KOOPA_RBO_MOD:
        riscv_printer.rem(cur, lhs, rhs);
        break;
    case KOOPA_RBO_AND:
        riscv_printer.and_(cur, lhs, rhs);
        break;
    case KOOPA_RBO_OR:
        riscv_printer.or_(cur, lhs, rhs);
        break;
    default:
        throw std::runtime_error("visit: invalid binary operator");
    }
    // 当前函数的 StackManager
    StackManager &stack_manager = context_manager.get_current_function_stack_manager();
    // 把结果存回栈中
    stack_manager.save_value_to_stack(value);
    riscv_printer.sw(cur, "sp", stack_manager.get_value_stack_offset(value), context_manager);
    // 当前结果所在的寄存器已经被使用过了, 释放
    context_manager.set_reg_free(value);
}
