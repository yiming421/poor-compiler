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
void visit_koopa(const koopa_raw_load_t& load, ofstream& fout);
void visit_koopa(const koopa_raw_store_t& store, ofstream& fout);
void visit_koopa(const koopa_raw_jump_t& jump, ofstream& fout);
void visit_koopa(const koopa_raw_branch_t& branch, ofstream& fout);
void visit_koopa(const koopa_raw_call_t& alloc, ofstream& fout);
void visit_koopa(const koopa_raw_get_elem_ptr_t& gep, ofstream& fout);
void visit_koopa(const koopa_raw_get_ptr_t& getptr, ofstream& fout);
void visit_global(const koopa_raw_value_t& alloc, ofstream& fout);
void print_init(const koopa_raw_value_t& init, ofstream& fout);
size_t getsize(const koopa_raw_type_t& type);
