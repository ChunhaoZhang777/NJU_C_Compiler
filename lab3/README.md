# Lab3 中间代码生成

本实验在前端检查成功后，把 C-- 整数程序翻译为课程规定的线性三地址代码，写入指定 `.ir` 文件。生成 IR 的入口是 `TranslateProgramToIr()`，其内部按文法递归处理表达式、语句和函数。

## 构建与运行

只运行本实验时，进入自身的 `Code` 目录，依次执行 `make` 和 `make test`；无需构建其他实验，也无需项目外层的测试文件。

以下命令均在项目根目录 **`NJU_C_Compiler`** 下执行。需要 GCC、GNU Make、Flex、Bison、m4，以及用于测试的 Python 3.8 以上版本。

```bash
sudo apt-get install build-essential flex bison m4 python3
make -C lab3/Code
./lab3/Code/parser lab3/test/valid/integer_arithmetic.cmm /tmp/lab3-sample.ir
cat /tmp/lab3-sample.ir
```

程序接受输入路径、输出路径两个参数。成功返回 `0`；参数、分析或文件读写错误返回 `1`。词法、语法、语义检查失败时不打开输出文件，已有文件不会因此被覆盖。

IR 包含 `FUNCTION`、`PARAM`、`ARG`、`CALL`、`RETURN`、`LABEL`、`GOTO`、`IF`、`DEC`、`READ`、`WRITE` 等指令。下面是格式示意，临时变量名和展开方式由翻译过程决定：

```text
FUNCTION main :
_t1 := #7
WRITE _t1
RETURN #0
```

## 翻译范围与约定

按实验约定处理无全局变量使用、变量不重名的整数程序；支持赋值、算术、条件、循环、短路逻辑、递归和数组访问，保留数组/结构体传参、多维数组及聚合类型复制扩展。浮点运算、函数声明以及数组/结构体返回值不属于本阶段的输入范围。

数组和结构体作为参数时传递地址；`ARG` 按逆序输出。条件翻译通过标签保证 `&&`、`||` 的短路求值。常量算术只在无副作用且结果可表示为有符号 32 位整数时折叠，溢出与除零保留为运行时运算。聚合类型赋值按对应元素、字段及各自偏移复制，较短数组复制完成后停止。

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
| `ir.c/.h` | 三地址指令、链表拼接、临时变量与标签命名及 IR 打印 |
| `ir_translate.c/.h` | 将通过语义检查的程序翻译成 IR，处理表达式、控制流和聚合类型地址 |
| `Makefile` | 生成、独立编译并链接词法与语法分析器，提供测试和清理入口 |

生成的 `parser.tab.c/.h`、`lexer.yy.c`、`parser.output`、`*.o` 和 `*.d` 统一放在 **`Code/build/`**，可执行程序保留为 **`Code/parser`**。例如 `build/main.d` 是 GCC 生成的依赖记录，不是 C 源码；它帮助 Make 在头文件变化后重新编译有关模块。

项目的 `.gitignore` 忽略 `build/`、可执行程序、缓存及 `Code` 内的临时 IR/汇编输出；`test` 中的 `.ir` 等样例仍可提交。需要删除构建产物时，执行 `make -C lab3/Code clean`。生成文件应通过修改 `.l`、`.y` 后重新构建，不直接编辑。

## 阅读代码

建议先读 `main.c` 和头文件了解阶段接口，再读 `symbol_table.c`、`semantic.c`，最后阅读本实验新增模块。语法树使用 `first_child`、`next`、`parent` 表示首个孩子、下一个兄弟和父节点。`FindSyntaxChild()` 按语法单元名查找直接孩子。

语法树节点拥有词素副本，符号表和 IR 可借用这些字符串。因此驱动程序在所有输出完成后调用 `ReleaseSyntaxTree()`；它沿独立分配链回收节点，也覆盖语法错误恢复时丢弃的节点。其他类型和 IR 对象仍按单次编译进程分配，不应把当前接口直接当作可重复调用的常驻编译服务。

## 测试

```bash
make -C lab3/Code test
```

当前运行 92 项检查。除公共前端及语义检查外，用项目的 `test/ir_vm.py` 执行生成的 IR，对比递归、短路求值、参数传递、名字冲突和聚合类型复制等结果，并检查常量折叠与错误输入不覆盖输出文件。

`test/valid`、`test/invalid` 中重新编写了 10 个具名源程序，`test/cases.json` 给出 12 项输入输出或诊断检查。覆盖整数运算、循环求和、短路副作用、数组、结构体布局、多维寻址和递归，详细运行方法见 [测试说明](test/README.md)。

课程运行测试来自 `Manual/Tests_3_s.zip` 的 16 个程序、25 组输入输出，预期结果保存在 `test/official_cases.json`。`ir_vm.py` 是有步数上限的测试解释器，入口供回归脚本调用，无需旧版 Python 2 模拟器。

## 实验资料

- [实验要求](Manual/实验三——中间代码生成.pdf)
- [实验指导](Manual/实验三指导.pdf)
- [课程测试说明](Manual/Tests.pdf)
- [模拟器附录](Manual/附录B.pdf)
- [项目说明](../README.md)
