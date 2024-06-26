#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
using std::string;
using std::pair;
using std::vector;
using std::unordered_map;

class GlobalSymbolTable {
public:
    GlobalSymbolTable();
    bool insert_func(string& name, bool flag);
    bool isExist_func(string& name) const;
    bool insert_var(string& name, int num,bool is_const);
    bool isExist_var(string& name) const;
    bool ret_func(string& name);
    unordered_map<string, bool> func_table; // bool: true for return, false for void
    unordered_map<string, pair<int, bool>> var_table; // bool: true for const, false for var
};

class SymbolTable {
public:
    bool insert(string& name, int num);
    bool insert(string& name, bool is_ptr, int len);
    bool isExist(string& name) const;
    int get(string& name);
    bool isConst(string& name);
    void push();
    void pop();
    int getID(string& name) const;
    void clear();
    GlobalSymbolTable gst;

private:
    pair<unordered_map<string, pair<int, bool>>, int> table; // int for id
    vector<pair<unordered_map<string, pair<int, bool>>, int>> st; // stack for table
    int id = 2; // avoid conflict with global table
    int len = 0;
};