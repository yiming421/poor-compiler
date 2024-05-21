#include <iostream>
#include <unordered_map>
#include <cassert>
#include "symbol.h"

bool SymbolTable::insert(std::string& name, int num) {
    auto& tab = table.first;
    if (tab.find(name) == tab.end()) {
        tab[name] = std::make_pair(num, true);
        return true;
    }
    return false;
}

bool SymbolTable::insert(std::string& name) {
    auto& tab = table.first;
    if (tab.find(name) == tab.end()) {
        tab[name] = std::make_pair(0, false);
        return true;
    }
    return false;
}

bool SymbolTable::isExist(std::string& name) const {
    auto& tab = table.first;
    if (tab.find(name) != tab.end()) {
        return true;
    }
    for (int i = len - 1; i >= 0; i--) {
        auto& t = st[i].first;
        if (t.find(name) != t.end()) {
            return true;
        }
    }
    return false;
}

int SymbolTable::get(std::string& name) {
    auto &tab = table.first;
    if (tab.find(name) != tab.end()) {
        return tab[name].first;
    }
    for (int i = len - 1; i >= 0; i--) {
        auto& t = st[i].first;
        if (t.find(name) != t.end()) {
            return t[name].first;
        }
    }
    assert(false);
    return -1;
}

bool SymbolTable::isConst(std::string& name) {
    auto& tab = table.first;
    if (tab.find(name) != tab.end()) {
        return tab[name].second;
    }
    for (int i = len - 1; i >= 0; i--) {
        auto& t = st[i].first;
        if (t.find(name) != t.end()) {
            return t[name].second;
        }
    }
    assert(false);
    return false;
}

int SymbolTable::getID(std::string& name) const {
    auto& tab = table.first;
    if (tab.find(name) != tab.end()) {
        return table.second;
    }
    for (int i = len - 1; i >= 0; i--) {
        auto& t = st[i].first;
        if (t.find(name) != t.end()) {
            return st[i].second;
        }
    }
    assert(false);
    return -1;
}

void SymbolTable::push() {
    st.push_back(table);
    id++;
    len++;
    table.first.clear();
    table.second = id;
} // performance

void SymbolTable::pop() {
    table = std::move(st.back());
    len--;
    st.pop_back();
} // performances

