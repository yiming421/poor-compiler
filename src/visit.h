#pragma once
#include <fstream>
#include <iostream>
#include "koopa.h"

using namespace std;

void visit_koopa(const koopa_raw_program_t& raw, ofstream& fout);
void visit_koopa(const koopa_raw_function_t& func, ofstream& fout);
void visit_koopa(const koopa_raw_basic_block_t& bb, ofstream& fout);
void visit_koopa(const koopa_raw_value_t& value, ofstream& fout);
void visit_koopa(const koopa_raw_return_t& ret, ofstream& fout);
int visit_koopa(const koopa_raw_integer_t& integer, ofstream& fout);
void visit_koopa(const koopa_raw_binary_t& binary, ofstream& fout);
