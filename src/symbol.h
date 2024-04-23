#pragma once
#include <iostream>
#include <unordered_map>


class SymbleTable {
public:
    bool insert(std::string name, int num);
    bool insert(std::string name);
    bool isExist(std::string name) const;
    int get(std::string name);
    bool isConst(std::string name);

private:
    std::unordered_map<std::string, std::pair<int, bool>> table;
    bool flag;
};