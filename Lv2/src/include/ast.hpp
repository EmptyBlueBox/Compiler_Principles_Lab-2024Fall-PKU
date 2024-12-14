/**
 * @file include/ast.hpp
 * @brief 定义了前端 AST 的结构体和类; 实现了中端 koopa 的输出
 * @note 前端: 通过词法分析和语法分析, 将源代码解析成抽象语法树 (abstract syntax tree, AST). 通过语义分析, 扫描抽象语法树, 检查其是否存在语义错误.
 * @note 中端: 将抽象语法树转换为中间表示 (intermediate representation, IR), 并在此基础上完成一些机器无关优化.
 * @note 后端: 将中间表示转换为目标平台的汇编代码, 并在此基础上完成一些机器相关优化.
 * @author Yutong Liang
 * @date 2024-11-13
 */

#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <sstream>

/**
 * @brief 所有抽象语法树的参数结构体
 * @details 这个结构体中包含两个成员变量, 一个字符串 mode 用于指定输出模式, 一个 ostringstream 指针用于存储输出结果
 * @note 我定义了这个结构体, 是为了避免在继承函数中添加参数的时候需要修改所有继承函数, 现在只需要修改这个结构体即可
 * @note 这个结构体中使用 ostringstream 指针, 是为了避免中端将 Koopa IR 输出到硬盘之后让后端再从硬盘读取, 现在只需要让后端从 ostringstream 内存中读取即可
 * @author Yutong Liang
 * @date 2024-11-13
 */
struct ASTPrintParam
{
    std::string mode;
    std::ostringstream *output_stream;
};

/**
 * @brief 所有抽象语法树的基类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    virtual void print(const ASTPrintParam &param) const = 0;
};

/**
 * @brief 程序单元抽象语法树类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    void print(const ASTPrintParam &param) const override;
};

/**
 * @brief 函数定义抽象语法树类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    void print(const ASTPrintParam &param) const override;
};

/**
 * @brief 函数类型抽象语法树类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class FuncTypeAST : public BaseAST
{
public:
    std::string type;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    void print(const ASTPrintParam &param) const override;
};

/**
 * @brief 块抽象语法树类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    void print(const ASTPrintParam &param) const override;
};

/**
 * @brief 语句抽象语法树类
 * @author Yutong Liang
 * @date 2024-10-27
 */
class StmtAST : public BaseAST
{
public:
    int number;
    /**
     * @brief 打印抽象语法树
     * @param[in] param.mode 打印模式
     * @param[out] param.output_stream 输出流
     * @author Yutong Liang
     * @date 2024-10-27
     */
    void print(const ASTPrintParam &param) const override;
};
