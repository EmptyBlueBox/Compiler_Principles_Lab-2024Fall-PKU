#include "include/koopa_util.hpp"

void KoopaContextManager::new_symbol_table_hierarchy()
{
    symbol_tables.push_back(std::unordered_map<std::string, Symbol>());
}

void KoopaContextManager::delete_symbol_table_hierarchy()
{
    symbol_tables.pop_back();
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
    _is_symbol_allocated_in_this_level[std::make_pair(name, symbol_tables.size())] = true;
}

bool KoopaContextManager::is_symbol_allocated_in_this_level(const std::string &name)
{
    return _is_symbol_allocated_in_this_level[std::make_pair(name, symbol_tables.size())];
}
