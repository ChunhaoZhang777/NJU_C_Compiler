# Lab4 MIPS32 目标代码生成

本实验在词法、语法和语义检查通过后，先生成 IR，再由 `TranslateIrToMips()` 完成指令选择、栈帧分配和函数调用翻译，输出可供 SPIM 执行的 MIPS32 汇编。

## 构建与运行

只运行本实验时，进入自身的 `Code` 目录，依次执行 `make` 和 `make test`；无需构建其他实验，也无需项目外层的测试文件。

以下命令均在项目根目录 **`NJU_C_Compiler`** 下执行。构建需要 GCC、GNU Make、Flex、Bison 和 m4；测试需要 Python 3.8 以上版本，运行汇编需要 SPIM。

```bash
sudo apt-get install build-essential flex bison m4 python3 spim
make -C lab4/Code
./lab4/Code/parser lab4/test/valid/recursive_fibonacci.cmm /tmp/lab4-fibonacci.s
printf '10\n' | spim -file /tmp/lab4-fibonacci.s
```

上面的样例读取一个整数并计算斐波那契数，输入 `10` 时应打印 `55`。SPIM 自身还会显示版本、加载信息和输入提示。

编译器接受输入路径、输出路径两个参数。成功返回 `0`，参数、分析或文件读写错误返回 `1`。前端检查失败时不打开输出文件；汇编运行结果由 SPIM 输出。编译和运行是两个独立步骤。

## 实现范围与调用约定

输入遵循实验中的整数、无全局变量使用、变量不重名、无函数声明及无聚合返回值约定。保留 IR 阶段的数组、结构体传参和多维数组扩展。输出包含 `read`、`write` 的 syscall 运行库，以及各函数对应的汇编标签。

- 每个函数先扫描 IR，确定局部变量和临时变量的固定栈位置，再发出函数序言。
- 前四个参数通过 `$a0`–`$a3` 传递，其余参数通过调用者栈传递；返回值保存在 `$v0`。
- 调用者保存 `$ra`，被调用者保存、恢复 `$s8`，返回时恢复 `$sp`，从而支持嵌套调用和递归。
- 栈偏移表用完整变量名处理哈希冲突；大于 16 位立即数范围的栈帧分配通过寄存器完成。

这是本编译器与 SPIM 配合使用的调用约定，未实现与外部 MIPS 系统库的 ABI 对接。

## 文件职责与构建产物

`Code` 保存手写源码，`Manual` 保存课程要求、指导和测试附件，`test` 保存可直接阅读、运行的源程序样例。

| 文件 | 职责 |
| --- | --- |
| `main.c` | 校验命令行参数，依次运行各编译阶段并检查文件读写结果 |
| `frontend.h` | 声明 Flex/Bison 接口与前端错误状态，避免在业务代码中重复声明 `extern` |
| `lexer.l` | 词法规则、数字识别、注释状态和词法诊断 |
| `parser.y` | 文法、运算符优先级、建树动作及错误恢复点 |
| `syntax_tree.c/.h` | 创建、查找、连接语法树节点及统一释放节点和词素副本 |
| `symbol_table.c/.h` | 变量、结构体、函数符号表和类型等价判断 |
| `semantic.c/.h` | 错误类型 1–17 的语义分析，公开整个程序的分析入口 |
| `ir.c/.h` | 三地址指令、操作数命名及指令链表 |
| `ir_translate.c/.h` | 由语法树生成目标代码选择所需的 IR |
| `mips.c/.h` | 指令选择、运行库、函数调用、栈帧分配及 MIPS32 汇编输出 |
| `Makefile` | 生成、独立编译并链接词法与语法分析器，提供测试和清理入口 |

生成的 `parser.tab.c/.h`、`lexer.yy.c`、`parser.output`、`*.o` 和 `*.d` 统一放在 **`Code/build/`**，可执行程序保留为 **`Code/parser`**。例如 `build/main.d` 是 GCC 生成的依赖记录，不是 C 源码；它帮助 Make 在头文件变化后重新编译有关模块。

项目的 `.gitignore` 忽略 `build/`、可执行程序、缓存及 `Code` 内的临时 IR/汇编输出；`test` 中的 `.ir` 等样例仍可提交。需要删除构建产物时，执行 `make -C lab4/Code clean`。生成文件应通过修改 `.l`、`.y` 后重新构建，不直接编辑。

## 阅读代码

建议先读 `main.c` 和头文件了解阶段接口，再读 `symbol_table.c`、`semantic.c`，最后阅读本实验新增模块。语法树使用 `first_child`、`next`、`parent` 表示首个孩子、下一个兄弟和父节点。`FindSyntaxChild()` 按语法单元名查找直接孩子。

语法树节点拥有词素副本，符号表和 IR 可借用这些字符串。因此驱动程序在所有输出完成后调用 `ReleaseSyntaxTree()`；它沿独立分配链回收节点，也覆盖语法错误恢复时丢弃的节点。其他类型和 IR 对象仍按单次编译进程分配，不应把当前接口直接当作可重复调用的常驻编译服务。

## 测试

```bash
make -C lab4/Code test
python3 lab4/test/run_tests.py --require-spim
```

第一个命令构建并运行本阶段测试；找不到 SPIM 时会提示跳过汇编执行。第二个命令强制要求真实 SPIM，找不到模拟器则失败，适合核对目标代码的实际执行结果。模拟器可通过环境变量指定：

```bash
SPIM='spim -exception_file /path/to/exceptions.s' python3 lab4/test/run_tests.py --require-spim
```

SPIM 可用时，当前运行 69 项检查，覆盖递归、超过四个参数、短路逻辑、大栈帧、数组/结构体复制和本实验的输入输出。回归脚本逐次执行生成的汇编并对比整数输出。

`test/valid`、`test/invalid` 中重新编写了 10 个具名源程序，`test/cases.json` 给出 15 项输入输出或诊断检查，重点覆盖递归调用的栈恢复、八参数调用、大栈偏移、输入输出和聚合类型复制。详细运行方法见 [测试说明](test/README.md)。

## 实验资料

- [实验要求](Manual/实验四——目标代码生成.pdf)
- [实验指导](Manual/实验四指导.pdf)
- [与 Lab3 共用的课程运行测试](../lab3/Manual/Tests.pdf)
- [项目说明](../README.md)
