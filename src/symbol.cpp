#include <iostream>
#include <unordered_map>
#include "symbol.h"

bool SymbleTable::insert(std::string name, int num) {
    if (table.find(name) == table.end()) {
        table[name] = std::make_pair(num, true);
        return true;
    }
    flag = true;
    return false;
}

bool SymbleTable::insert(std::string name) {
    if (table.find(name) == table.end()) {
        table[name] = std::make_pair(0, false);
        return true;
    }
    flag = false;
    return false;
}

bool SymbleTable::isExist(std::string name) const {
    return table.find(name) != table.end();
}

int SymbleTable::get(std::string name) {
    return table[name].first;
}

bool SymbleTable::isConst(std::string name) {
    return table[name].second;
}

