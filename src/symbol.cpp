#include <iostream>
#include <unordered_map>
#include <cassert>
#include "symbol.h"

using std::string;
using std::pair;
using std::vector;
using std::unordered_map;
using std::make_pair;

bool SymbolTable::insert(std::string& name, int num) {
    auto& tab = table.first;
    if (tab.find(name) == tab.end()) {
        tab[name] = std::make_pair(num, true);
        return true;
    }
    return false;
}

bool SymbolTable::insert(std::string& name, bool is_ptr, int len) {
    auto& tab = table.first;
    if (tab.find(name) == tab.end()) {
        if (is_ptr) {
            tab[name] = std::make_pair(len, false);
        } else { // we use negative number to represent the length of array
            tab[name] = std::make_pair(-len, false);
        }
        return true;
    }
    return false;
}

bool SymbolTable::isExist(std::string& name) const {
    auto& tab = table.first;
    if (tab.find(name) != tab.end()) {
        return true;
    } // current table
    for (int i = len - 1; i >= 0; i--) {
        auto& t = st[i].first;
        if (t.find(name) != t.end()) {
            return true;
        }
    } // previous tables
    if (gst.isExist_var(name)) {
        return true;
    } // global table
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
    if (gst.isExist_var(name)) {
        return gst.var_table[name].first;
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
    if (gst.isExist_var(name)) {
        return gst.var_table[name].second;
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
    if (gst.isExist_var(name)) {
        return 0;
    }
    assert(false);
    return -1;
}

void SymbolTable::push() {
    st.push_back(table);
    len++;
    table.first.clear();
    table.second = id;
    id++;
}

void SymbolTable::pop() {
    table = std::move(st.back()); // move the table to the top
    len--; // only len decreases, not id 
    st.pop_back();
}

void SymbolTable::clear() {
    table.first.clear();
    table.second = id;
    id += 1;
}

GlobalSymbolTable::GlobalSymbolTable() { // initialize the function table
    func_table["getint"] = true;
    func_table["getch"] = true;
    func_table["getarray"] = true;
    func_table["putint"] = false;
    func_table["putch"] = false;
    func_table["putarray"] = false;
    func_table["starttime"] = false;
    func_table["stoptime"] = false;
}

bool GlobalSymbolTable::insert_func(string& name, bool flag) {
    if (func_table.find(name) != func_table.end()) {
        return false;
    }
    func_table[name] = flag;
    return true;
}

bool GlobalSymbolTable::ret_func(string& name) {
    return func_table[name];
}

bool GlobalSymbolTable::isExist_func(string& name) const {
    return func_table.find(name) != func_table.end();
}

bool GlobalSymbolTable::insert_var(string& name, int num, bool is_const) {
    if (var_table.find(name) != var_table.end()) {
        return false;
    }
    if (is_const) {
        var_table[name] = std::make_pair(num, true);
    } else {
        var_table[name] = std::make_pair(-num, false);
    }
    return true;
}

bool GlobalSymbolTable::isExist_var(string& name) const {
    return var_table.find(name) != var_table.end();
}
