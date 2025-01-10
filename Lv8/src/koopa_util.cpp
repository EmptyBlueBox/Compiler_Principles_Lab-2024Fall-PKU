#include "include/koopa_util.hpp"

void KoopaContextManager::new_symbol_table_hierarchy()
{
    symbol_tables.push_back(std::unordered_map<std::string, Symbol>());
}

void KoopaContextManager::delete_symbol_table_hierarchy()
{
    symbol_tables.pop_back();
}

bool KoopaContextManager::is_global()
{
    return symbol_tables.size() == 1;
}

void KoopaContextManager::insert_symbol(const std::string &name, Symbol symbol)
{
    if (symbol_tables.empty())
    {
        throw std::runtime_error("KoopaContextManager::insert_symbol: symbol table is empty");
    }
    symbol_tables.back()[name] = symbol;
}

Symbol KoopaContextManager::name_to_symbol(const std::string &name)
{
    for (int i = symbol_tables.size() - 1; i >= 0; --i)
    {
        if (symbol_tables[i].find(name) != symbol_tables[i].end())
        {
            Symbol symbol = symbol_tables[i].at(name);
            if (symbol.type == Symbol::Type::VAL)
            {
                return symbol;
            }
            else if (symbol.type == Symbol::Type::VAR)
            {
                return Symbol(Symbol::Type::VAR, i + 1);
            }
            else
            {
                throw std::runtime_error("KoopaContextManager::read: invalid symbol type");
            }
        }
    }
    throw std::runtime_error("KoopaContextManager::read: identifier does not exist");
}

void KoopaContextManager::set_symbol_allocated_in_this_level(const std::string &name)
{
    // 如果在刚进入函数这一层, 就不要设置这个 symbol 被分配, 避免以下情况:
    // void func1(int x) {
    //     int y;
    // }
    // void func2(int x) {
    //     int y;
    // }
    // 如果不做这个特判, 第二个函数中的 y 就不会被分配空间
    if (symbol_tables.size() <= 2)
        return;
    _is_symbol_allocated_in_this_level[std::make_pair(name, symbol_tables.size())] = true;
}

bool KoopaContextManager::is_symbol_allocated_in_this_level(const std::string &name)
{
    return _is_symbol_allocated_in_this_level[std::make_pair(name, symbol_tables.size())];
}
