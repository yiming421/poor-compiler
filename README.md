# （可怜的）SysY编译器项目报告

## 1 编译器概述

### 1.1 基本功能

本编译器基本具备以下功能：
1 该编译器可以把 SysY 语言编译到 RISC-V 汇编。
2 编译器首先将 SysY 语言编译到 Koopa IR ，再将 Koopa IR 翻译成 RISC-V 汇编。
3 通过指定 `-koopa` 可生成 Koopa IR ，指定 `-riscv` 可生成 RISC-V。执行指令

```sh
make
cd build
./compiler -koopa SysY路径 -o KoopaIR路径
./compiler -riscv SysY路径 -o KoopaIR路径
```

### 1.2 主要特点
本编译器的主要特点是：
+ **三层中间表示**：上层的中间表示为抽象语法树，中层的中间表示是 Koopa IR，下层的中间表示是内存形式 Koopa IR ，我们通过词法分析和语法分析得到抽象语法树，在通过遍历抽象语法树生成Koopa IR，最后调用 Koopa IR 接口生成内存形式 Koopa IR。
+ **前后端低耦合**：前端只对后端提供 Koopa IR 表示，没有共享的数据结构。

## 2 编译器设计

### 2.1 主要模块组成

编译器由4个主要模块构成：

+ **词法分析模块**: 通过词法分析，将SysY程序转换为token流。(`sysy.l`)
+ **语法分析模块**: 通过语法分析，得到抽象语法树。(`sysy.y`)
+ **IR生成模块**: 遍历抽象语法树，进行语义分析，得到 Koopa IR 中间表示。(`ast.[h|cpp]`)
+ **代码生成模块**: 扫描Koopa IR内存形式，得到RISC-V代码。(`visit.[h|cpp]`)


另外还引入了一些辅助模块。如symbol模块定义了生成IR所需的符号表，util模块定义了打印指令的函数。

### 2.2 主要数据结构

#### 2.2.1 抽象语法树

`ast.[h\cpp]` 实现了抽象语法树，由基类 `BaseAST` 派生出一系列子类：

```cpp
class BaseAst {
protected:
    static int id;
public:
    int idx = 0;
    int num = 0;
    string ident;
    virtual ~BaseAst() = default;
    virtual void dump(std::stringstream& out) {};
    virtual int cal() { return 0; };
};
```

idx,num,ident等变量在子类中频繁出现，为适应子类的需求，在基类中添加。
由dump函数生成Koopa IR代码，由于有时需要传递额外信息，子类中dump额外重载，调用时需显式类型转换。
cal函数主要出现在表达式中，用于常量传播与常量折叠，同时也有额外重载。
子类的一个例子：

```cpp
class ConstInitValAst : public BaseAst {
public:
    std::unique_ptr<BaseAst> const_exp;
    std::unique_ptr<BaseAst> const_init_val_list;
    int cal();
    void dump(std::stringstream& out, string& ident, vector<int>& nums, bool flag); // print init value
    void cal(vector<int>& nums, int idx, int& cnt, vector<int>& data); // helper function
    void dump(std::stringstream& out) {}
};
```

#### 2.2.2 符号表

符号表由 `symbol.[h|cpp]` 实现，结构如下：

```cpp
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
```

其中， `GlobalSymbolTable` 为全局符号表，负责处理全局符号，结构如下：

```cpp
class GlobalSymbolTable {
public:
    GlobalSymbolTable();
    bool insert_func(string& name, bool flag);
    bool isExist_func(string& name) const;
    bool insert_var(string& name, int num, bool is_const);
    bool isExist_var(string& name) const;
    bool ret_func(string& name);
    unordered_map<string, bool> func_table; // bool: true for return, false for void
    unordered_map<string, pair<int, bool>> var_table; // bool: true for const, false for var
};
```

符号表的实现细节，下文中将略加介绍。

#### 2.2.3 栈帧

`visit.cpp` 中定义了栈帧模拟器类 `Stack`，用以管理各变量偏移量，结构如下：

```cpp
class stack {
public:
    stack() {}
    int precompute(const koopa_raw_function_t& func);
    int getbias(const koopa_raw_value_t& value);
    int clear();
    int getnum();
private:
    int bias = 0;
    int params_num = 0;
    unordered_map<koopa_raw_value_t, int> var2bias;
}; 
```

由于结构较简单，并没有单开文件实现。var2bias记录了变量到偏移量的对应，而对应在为函数生成代码时，由precompute方法计算而来。
precompute对每一个Koopa IR语句中的变量计算偏移，并保存在var2bias中。
值得特别注意的是函数参数处理部分，该部分的细节较多，实现较为繁琐。

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑

结合符号表的结构具体说明：

```cpp
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
```

首先来看变量 `pair<unordered_map<string, pair<int, bool>>, int> table` 。pair.second是每一个table对应的id，
这样能保证不同作用域使用的相同变量名能得到区分。后面的map将符号名映到另一个pair，pair.second用以区别变量和常量，
对于常量，pair.first用以记录常量的值；对于变量，pair.first用以记录数组的长度（不是数组则为0）。
注意，我对指针数组和数组做了特殊处理。指针数组pair.first域为正，而数组为负。这是由于Koopa IR对指针数组和数组处理上有细节的不同，
必须加以区分。
变量 `st` 则为栈，用以管理嵌套的作用域，容纳了暂时不活跃的符号表。 `push` `pop` 用于插入和删除，
我们使用std::move来加速作用域的转移过程。
除此之外，符号表提供了众多功能函数。基本的逻辑是，首先在当前符号表查找符号，若查询失败前往栈中查找符号，最后查找全局符号。
接下来看全局符号表：

```cpp
class GlobalSymbolTable {
public:
    GlobalSymbolTable();
    bool insert_func(string& name, bool flag);
    bool isExist_func(string& name) const;
    bool insert_var(string& name, int num, bool is_const);
    bool isExist_var(string& name) const;
    bool ret_func(string& name);
    unordered_map<string, bool> func_table; // bool: true for return, false for void
    unordered_map<string, pair<int, bool>> var_table; // bool: true for const, false for var
};
```

全局符号表与符号表基本类似，不同之处在于全局符号表还需考虑函数。采用 `func_table` 与 `var_table` 分别处理函数和变量。
`func_table` 将符号定义映到返回类型，由于（可怜的）SysY编译器不检查函数类型是否匹配（真可怜），不需记录额外信息。
但我们仍需要记录函数的返回类型。返回类型不同的情况下，对于return语句需要有不同的处理。

#### 2.3.2 寄存器分配策略

（可怜的）SysY编译器选择了全扔到栈上，这种实现方法的优势在于，（可怜的）程序员的脑力开销最小。

#### 2.3.3 采用的优化策略

首先注意到，（可怜的）SysY编译器对函数并不进行类型检查，既然如此，为什么还要保存函数的参数信息呢？
我决定在参数表里去掉参数信息，成功节约了大量的传参开销。
其次RISC-V中，很多指令只支持imm12，限制了常数的范围，我们被迫要首先将常数加载到寄存器中，制造了额外的开销。
但实际上，超imm32范围的常数并没有那么多，对于每一种常数都去加载到寄存器中并没有意义。于是在生成RISC-V代码时，
额外判断常数的范围，对于在imm12范围内的常数，不加载到寄存器中，节省了这部分开销。
同时注意到，全局数组初始化时，如果初始化数据全为0，没有必要生成初始化列表。在代码中，我添加了对这种情况的检测。
如果全局数组的初始化列表中只有一小部分值非零，我们在生成RISC-V代码时，直接对大块的零值用.zero批量处理。
遗憾的是，如果初始化数据不全为零，Koopa IR要求生成完整的初始化列表，这阻碍了进一步优化；
同时，对于本地数组的初始化，这种优化思路不起作用，由于本地数组必须进行完全的初始化，而Koopa IR并没有对大块区域填充的指令。

#### 2.3.4 其他补充设计考虑

对于临时寄存器的使用，我选择直接将寄存器编号作为继承属性，在ast节点间传递，这样不需要用符号表显式追踪所有临时寄存器，能节约大量开销；
尽量运用面向对象的方法设计编译器，将各部分功能隔离开来，如 `SymbolTable` 类与 `Printer` 类，生成代码过程中，只需要调用对应接口，
并且尽量将对应的工作全部放到辅助类中执行，有效地降低了设计的复杂度。
代码生成采用on-the-fly的策略，生成的IR直接放到stringstream中，生成的RISC-V代码直接输出到文件中。

## 3 编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成

万事开头难，这一阶段的难点主要在于快速熟悉环境和可用工具，比如Flex和Bison的语法，Docker Git的配置，Koopa IR及内存形式，
RISC-V的语法（以及imm12的偏移量限制），自定义测试用例。至于代码方面，唯一小有难度的点是注释块的处理，设计出正确的正则表达式并不那么平凡。
注释块的处理： `LineComment "//".*|"/*"([^\*]*|[\*]+[^\*\/])*[\*]+\/`

#### Lv3. 表达式

Lv1&&Lv2比较简单，Lv3相对更为复杂，需要处理众多的运算符，表达式。这里涉及到几个问题。
首先是前端的寄存器问题。Koopa IR是静态单赋值IR，不允许使用重复的变量，所以我们使用static的 `BaseAst::id` 来记录当前寄存器编号，
并且用 `idx` 保存该Ast节点的寄存器编号，并将其作为继承属性在抽象语法树上传递。
然后是后端的寄存器问题。由于（可怜的）程序员脑力资源有限，无法在Lv3解决寄存器分配问题，于是选择了直接将中间变量入栈的方案，制造了大量的load store。
最后需要注意LAnd和LOr。因为Koopa IR只支持按位逻辑运算，我们需要使用按位逻辑运算拼出逻辑运算。但由于短路求值，这部分代码会在之后发生变化。
一个简单的dump函数例子：

```cpp
void AddExpAst::dump(std::stringstream& out) {
    if (type != 0){
        add_exp->dump(out);
        mul_exp->dump(out);
        if (add_exp->idx == -1 && mul_exp->idx == -1) {
            idx = -1;
            if (op == "+"){
                num = add_exp->num + mul_exp->num;
            } else {
                num = add_exp->num - mul_exp->num;
            }
        } else {
            idx = BaseAst::id;
            printer.print_binary(idx, op, add_exp, mul_exp, out);
            BaseAst::id++;
        }
    } else {
        mul_exp->dump(out);
        idx = mul_exp->idx;
        num = mul_exp->num;
    }
}
```

#### Lv4. 常量与变量

这一阶段需要引入符号表和栈帧。关于符号表和栈帧，这里不再赘述。
这里imm12的范围限制问题首次出现。比如alloc栈帧的时候，可能越过imm12的范围。这里我们就需要手动将常数加载入寄存器中，如：

```cpp
    if (num >= -2048 && num <= 2047) {
        out << "  addi sp, sp, " << num << endl;
    } else {
        out << "  li t2, " << num << endl;
        out << "  add sp, sp, t2" << endl;
    }
```

对于变量列表，我们可以考虑在语法分析阶段直接将全部信息加载到第一个变量列表中，但我没有采取这样的做法，因为在语义分析阶段再进行分析并不会增加太多开销，
同时更加清晰明了。
对于常量，在语义分析阶段直接进行常量传播，常量折叠，我们采用cal函数来计算一个常量的值，如：

```cpp
int AddExpAst::cal() {
    if (type != 0){
        int num1 = add_exp->cal();
        int num2 = mul_exp->cal();
        if (op == "+"){
            return num1 + num2;
        } else {
            return num1 - num2;
        }
    } else {
        return mul_exp->cal();
    }
}
```

同时，对于一个Ast节点，我们将idx设为-1，以示其为常量，并用num保存常量的值。
还需要注意一个细节上的问题。当检查到return语句，无论后面还有什么变量定义，都不应该继续输出，否则不符合Koopa IR的规范。由于这一点，加入了全局变量end，
当检查到end语句后停止后续代码的生成，解决了这个问题。

#### Lv5. 语句块和作用域

要支持作用域，我们只需在符号表中维护一个栈，并在作用域的出入口弹进弹出。
符号表的pop与push方法实现为：

```cpp
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
```

需要特别注意的是，即便在嵌套作用域的内部遇到return语句，也应该立即停止生成，需要正确的处理这一点。

#### Lv6. if语句

进入到这部分，首先要处理的就是大名鼎鼎的dangling-else问题。如果直接这样写语义规则：

```M.Bison
IfStmt: IF '(' Exp ')' Stmt ELSE Stmt
```

就会由于if与else配对的多种可能陷入冲突。
课上给出了一种解决方案：我们重新定义一个Ast子类，这个子类中if和else一定要相互匹配，这样最近的if和else一定优先匹配，语义规则变成这样：

```M.Bison
Stmt: IfStmt | WithElse

IfStmt: IF '(' Exp ')' Stmt | IF '(' Exp ')' WithElse ELSE IfStmt

WithElse: IF '(' Exp ')' WithElse ELSE WithElse | OtherStmt
```

其余要处理的问题无非是在IR和RISC-V中的各种跳转，需要仔细的设计。加入短路求值之后，逻辑表达式的dump函数变为：

```cpp
void LAndExpAst::dump(std::stringstream& out) {
    if (type != 0){
        land_exp->dump(out);
        count.cnt++;
        string then_label = count.getlabel("then"); // implement short-circuit evaluation
        string end_label = count.getlabel("end");
        idx = BaseAst::id;
        string ident = "tmp_" + std::to_string(count.cnt);
        table.insert(ident, idx);
        printer.print_alloc(ident, out, table);
        printer.print_store(false, 0, ident, out, table);
        if (land_exp->idx == -1) {
            printer.print_br(false, land_exp->num, then_label, end_label, out);
        } else {
            printer.print_br(true, land_exp->idx, then_label, end_label, out);
        }
        printer.print_label(then_label, out);
        eq_exp->dump(out);
        idx = BaseAst::id;
        printer.print_eq(false, idx, eq_exp, out);
        printer.print_store(true, idx, ident, out, table);
        printer.print_jump(end_label, out);
        printer.print_label(end_label, out);
        printer.print_load(idx + 1, ident, out, table);
        BaseAst::id += 2;
        idx += 1;
    } else {
        eq_exp->dump(out);
        idx = eq_exp->idx;
        num = eq_exp->num;
    }
}
```

最后一个容易忽视的点是，if/else中即使出现了return语句，也不能说明函数已经结束了，当return语句出现在分支时，我们不能认为函数已经结束。但同时，
return语句如果出现，不能不做任何处理，因为return本身就会改变控制流的方向，与jump的作用是重复的。所以这里需要这样的处理：

```cpp
if (!end) { // only print jump when there is no return statement
    printer.print_jump(end_label, out);
} else {
    end = false;
}
```

#### Lv7. while语句

while语句和if语句差别不大，唯一的不同是，我们需要找到break与continue语句的目标。这里我们仍然采用idx传播的方式解决，如：

```cpp
    case 6: { // continue stmt
        assert(idx != 0);
        string entry_label = count.getlabel("entry", idx);
        printer.print_jump(entry_label, out);
        end = true;
        break;
    }
```

#### Lv8. 函数和全局变量

这一阶段首先需要添加函数。添加函数时，因为（可怜的）SysY编译器不进行类型检查，所以符号表中不显式维护参数列表。但在函数体内部，我们需要重新加载一遍参数，
所以仍需取得全部参数的列表，并将其传入BlockAst中。这一部分代码处理为：

```cpp
    vector<pair<string, string>> params;
    if (func_fparams != nullptr) {
        reinterpret_cast<FuncFparamsAst&>(*func_fparams).dump(out, params);
    }
    reinterpret_cast<BlockAst&>(*block).dump(out, params); 
```

重新加载参数时，为与参数区分，我们只需要对加载的变量参数加一即可。
我们仍然需要特别注意返回值。每一次进入一个新的函数，应重置 `end` 变量。如果函数没有给出返回语句，应显式补全返回语句。
对于全局变量，遵照语法的具体声明实现即可。我们可以放心地使用标号0，但需要注意，由于全局变量和函数定义可以交叉，应防止全局变量名与参数名重合。
这一部分的后端设计，最重要的是恰当处理参数。需要谨慎地考察当参数超过8个时，多余参数在栈中的排放方式，并对偏移量进行恰当的计算。这一部分的代码有：

```cpp
    else if (value->kind.tag == KOOPA_RVT_CALL) {
        var2bias[value] = bias;
        bias += 4;
        flag_ra = true;
        if (value->kind.data.call.args.len > 8) { // if the number of parameters is greater than 8
        // then the parameters are stored in the stack
            params_num = std::max(params_num, static_cast<int>(value->kind.data.call.args.len));
        }
    }
    ......
    if (flag_ra) {
        bias += 4;
    } // if the function has a return value, then the return value is stored in the stack
    if (params_num > 8) {
        bias += 4 * (params_num - 8);
        for (auto& pair: var2bias) {
            var2bias[pair.first] += 4 * (params_num - 8); // update the bias of the parameters
        }
    }
```

全局变量注意应用la加载。

#### Lv9. 数组

在多维数组的实现中，初始化列表的处理是最麻烦的。生成Koopa IR时，对于全局变量，我们需要生成完整的初始化列表；对于局部变量，我们需要进行一系列填入。
在我的实现中，首先根据SysY中的初始化列表填好记录数组数据的data列表，再根据data列表生成IR。初始化列表遵循对齐原则，用递归函数加以填充，代码如：

```cpp
void ConstInitValListAst::cal(vector<int>& nums, int idx, int& cnt, vector<int>& data, bool flag) { // helper function for dump
    int cnt_now = cnt;
    reinterpret_cast<ConstInitValAst&>(*const_init_val).cal(nums, abs(idx), cnt, data);
    if (const_init_val_list != nullptr) {
        reinterpret_cast<ConstInitValListAst&>(*const_init_val_list).cal(nums, idx, cnt, data, false);
    }
    if (flag) { // Fill the rest with 0
        for (int i = cnt - cnt_now; i < nums[idx]; i++) {
            data.push_back(0);
        }
    cnt = cnt_now + nums[idx]; // Update the count
    }
}

void ConstInitValAst::cal(vector<int>& nums, int idx, int& cnt, vector<int>& data) {
    if (const_exp != nullptr) {
        data.push_back(const_exp->cal());
        cnt++;
    } else {
        assert(cnt % nums[nums.size() - 1] == 0); // the count should be a multiple of the last dimension
        int i = nums.size() - 2;
        for (; i > idx; i--) { // find the first dimension that is not a multiple of the count
            if (cnt % nums[i] != 0) {
                break;
            }
        }
        if (const_init_val_list == nullptr) {
            for (int k = 0; k < nums[i + 1]; ++k) { // fill the rest with 0
                data.push_back(0);
                cnt++;
            }
            return;
        }
        reinterpret_cast<ConstInitValListAst&>(*const_init_val_list).cal(nums, i + 1, cnt, data, true);
    }
}
```
生成Koopa IR的初始化列表，与局部变量的填充，也采用递归方式完成。
初始化列表生成如下：

```cpp
void Printer::recursive_print_agg_const(vector<int>& data, vector<int>& nums, int& i, int j , std::stringstream& out) { // print aggregation
    if (j == nums.size() - 1) {
        out << '{';
        for (int k = 0; k < nums[j]; k++) {
            if (k != 0) {
                out << ", ";
            }
            out << data[i++];
        }
        out << '}';
        return;
    }
    out << '{';
    for (int k = 0; k < nums[j]; k++) {
        if (k != 0) {
            out << ", ";
        }
        recursive_print_agg_const(data, nums, i, j + 1, out);
    }
    out << '}';
}
```

局部变量的填充如下：

```cpp
void Printer::recursive_print_init_arr_const(std::string& ident, vector<int>& data, vector<int>& nums, int& idx, std::stringstream& out, 
                                       SymbolTable& table, int& i, int j) { // recursively 
    if (j == nums.size() - 1) {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx  << ", " << k << std::endl;
            out << "  store " << data[i++] << ", %" << idx << std::endl;
            idx += 1;
        }
        return;
    } else {
        int oriidx = idx - 1;
        for (int k = 0; k < nums[j]; k++) {
            out << "  %" << idx << " = getelemptr %" << oriidx << ", " << k << std::endl;
            idx += 1;
            recursive_print_init_arr_const(ident, data, nums, idx, out, table, i, j + 1);
        }
    }
}
```

在传递数组参数的时候，涉及到了指针类型。指针类型需要用getptr操作，这需要我们区分数组和指针数组。我们通过符号表中len的正负来达到这一点。同时，
在将数组作为指针传入函数时，需要将数组转化为一个指针，我们需要一个偏移量为0的getptr指令。
储存与加载的过程中也要注意区分指针和数组，代码如：

```cpp
if (flag) {
    out << "  %" << idx << " = load @" << (ident + '_' + std::to_string(id)) << std::endl;
    out << "  %" << idx + 1 << " = getptr %" << idx << ", ";
    idx += 1;
} else {
    out << "  %" << idx << " = getelemptr @" << (ident + '_' + std::to_string(id)) << ", ";
}
```
后端实现时注意getelemptr与getptr的区别。如处理getelemptr的代码为：

```cpp
void visit_koopa(const koopa_raw_get_elem_ptr_t& gep, ofstream& fout) {
    if (gep.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        riscv_printer.print("  la t0, " + string(gep.src->name + 1), fout);
    } else if (gep.src->kind.tag == KOOPA_RVT_ALLOC) {
        riscv_printer.print("  li t0, " + to_string(st.getbias(gep.src)), fout);
        riscv_printer.print("  add t0, sp, t0", fout);
    } else {
        riscv_printer.print_load("t0", st.getbias(gep.src), fout);
    } // load the address of the source variable
    if (gep.index->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_printer.print_load_const("t1", gep.index->kind.data.integer.value, fout);
    } else {
        riscv_printer.print_load("t1", st.getbias(gep.index), fout);
    } // load the index
    size_t size = getsize(gep.src->ty->data.pointer.base->data.array.base);
    riscv_printer.print("  li t2, " + to_string(size), fout);
    riscv_printer.print("  mul t1, t1, t2", fout);
    riscv_printer.print("  add t0, t0, t1", fout); // calculate the address of the element
}
```

而getptr只需要对size的获取方式做一些小改动即可。

### 3.2 工具软件介绍

+ Flex：用于词法分析，用正则表达式得到token流。
+ Bison：用于语法分析的工具。与Flex配合使用，通过LR分析得到AST。
+ LibKoopa：用于Koopa IR转内存形式，便于中间代码生成。

### 3.3 测试情况说明

集中在Lv8和Lv9，本项目经历了漫长的测试。测试除了课程提供的丰富测试用例之外，主要还是手动测试一些可能的边界情况。一些边界情况包括：
1 如果寄存器采用identnum形式设计，如a1，a2，会出现一个问题，如果变量命名为a1，num为1，IR中则为a11，会与命名为a的变量名，num为11的情况发生冲突，课程提供的测试用例似乎并没有覆盖到这一点。
2 SysY的库函数的全部参数及返回类型，似乎课程的测试用例并不完全，比如我在本地测试时，发现原本的代码认为getarray没有返回值，但在线测试并没有发现这个问题。

## 4 实习总结

### 4.1 收获与体会

最初听说编译原理的实习作业任务量很大，很多同学在假期就开始做了，所以最开始有些紧张。因为我是后来补选的编译原理，假期没有做好准备，对手写一个编译器没什么信心。但到现在，做完了9个阶段以后，感觉这3000行代码，也并不是特别难写，前前后后花的时间可能还不到100h。所以对现在的我，对从零开始写一个大项目，可能会少一些抵触和畏惧，多一些自信与从容。说到底，从事信息行业的人最重要的能力就是码力，能不能快速利用代码技能，对可能遇到的种种问题，快速给出优雅的解决方案，是程序员的必备技能。当然，我可能和MaxXing（神）还差到不知道哪里，但和写计概大作业的nogo都手忙脚乱的菜鸟来比，还是有长足进步的。
同时，实习作业也让我对编译技术的深度有了一定认知。我们学习的编译理论只是冰山一角，而这些理论都没有充分应用在实践当中，更何况SysY只是C语言的一个子集，问题就已经十分复杂了。无论是排行榜上快了好几倍的榜一大哥，还是gnu恐怖的代码长度，都让我对现代软件开发的神奇感到惊叹。路漫漫而修远兮，这个世界上还有很多事物可供探索。

### 4.2 学习过程中的难点，以及对实习过程和内容的建议

不能说难点，但我的一个痛点是，我是按照lv数量在整个学期平滑地分配任务量的，但后来我才意识到，lv的任务量并不是平均分配的。所以后面的几周（lv8lv9）可能非常非常忙。为方便后来人科学合理地评估任务量，可以考虑合并前面的几个阶段（如Lv5），拆分后面的几个阶段（如Lv9），或者在实践地开头就指出，后面的几个阶段的任务量可能会比较大，我觉得这样会好一点。

注：可供考虑的测试用例

```cpp
int foo() {
    int a1 = 0;
    {int a = 1;}
    {int a = 2;}
    {int a = 3;}
    {int a = 4;}
    {int a = 5;}
    {int a = 6;}
    {int a = 7;}
    {int a = 8;}
    {int a = 9;}
    {int a = 10;}
    {int a = 11;}
    return a1;
}

int main() {
    starttime();
    int n = getint();
    putint(n);
    int c = getch();
    putch(c);
    int a[10][10];
    int n = getarray(a[0]);
    putarray(n, a[0]);
    stoptime();
    return foo();
}
```