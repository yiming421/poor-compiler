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
    if (gst.isExist_var(name)) {
        return true;
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
} // performance

void SymbolTable::pop() {
    table = std::move(st.back());
    len--;
    st.pop_back();
} // performances

void SymbolTable::clear() {
    table.first.clear();
    table.second = id;
    id += 1;
}

GlobalSymbolTable::GlobalSymbolTable() {
    vector<string> params0({});
    vector<string> params1({"i32"});
    vector<string> params2({"*i32"});
    vector<string> params3({"i32", "*i32"});
    func_table["getint"] = make_pair(params0, true);
    func_table["getch"] = make_pair(params0, true);
    func_table["getarray"] = make_pair(params2, false);
    func_table["putint"] = make_pair(params1, false);
    func_table["putch"] = make_pair(params1, false);
    func_table["putarray"] = make_pair(params3, false);
    func_table["starttime"] = make_pair(params0, false);
    func_table["stoptime"] = make_pair(params0, false);
}

bool GlobalSymbolTable::insert_func(string& name, vector<string>& params, bool flag) {
    if (func_table.find(name) != func_table.end()) {
        return false;
    }
    func_table[name] = make_pair(params, flag);
    return true;
}

bool GlobalSymbolTable::ret_func(string& name) {
    return func_table[name].second;
}

bool GlobalSymbolTable::isExist_func(string& name) const {
    return func_table.find(name) != func_table.end();
}

vector<string>& GlobalSymbolTable::getParams(string& name) {
    assert(func_table.find(name) != func_table.end());
    return func_table[name].first;
}

bool GlobalSymbolTable::insert_var(string& name, int num, bool is_const) {
    if (var_table.find(name) != var_table.end()) {
        return false;
    }
    var_table[name] = std::make_pair(num, is_const);
    return true;
}

bool GlobalSymbolTable::isExist_var(string& name) const {
    return var_table.find(name) != var_table.end();
}
