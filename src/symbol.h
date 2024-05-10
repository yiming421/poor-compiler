#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>

class SymbolTable {
public:
    bool insert(std::string name, int num);
    bool insert(std::string name);
    bool isExist(std::string name) const;
    int get(std::string name);
    bool isConst(std::string name);
    void push();
    void pop();
    int getID(std::string name) const;

private:
    std::pair<std::unordered_map<std::string, std::pair<int, bool>>, int> table;
    std::vector<std::pair<std::unordered_map<std::string, std::pair<int, bool>>, int>> st;
    int id = 0;
    int len = 0;
};