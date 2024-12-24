#include "include/util_koopa.hpp"

void SymbolTable::new_symbol_table_hierarchy()
{
    symbol_table.push_back(std::unordered_map<std::string, Symbol>());
}

void SymbolTable::delete_symbol_table_hierarchy()
{
    symbol_table.pop_back();
}

void SymbolTable::insert_symbol(const std::string &name, Symbol symbol)
{
    if (symbol_table.empty())
    {
        throw std::runtime_error("SymbolTable::insert_symbol: symbol table is empty");
    }
    symbol_table.back()[name] = symbol;
}

Symbol SymbolTable::read(const std::string &name)
{
    for (int i = symbol_table.size() - 1; i >= 0; --i)
    {
        if (symbol_table[i].find(name) != symbol_table[i].end())
        {
            Symbol symbol = symbol_table[i].at(name);
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
                throw std::runtime_error("SymbolTable::read: invalid symbol type");
            }
        }
    }
    throw std::runtime_error("SymbolTable::read: identifier does not exist");
}

void SymbolTable::set_returned(bool is_returned)
{
    this->is_returned = is_returned;
}

bool SymbolTable::get_returned()
{
    return this->is_returned;
}
