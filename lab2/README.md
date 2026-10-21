# Lab2 语义分析

本实验在词法、语法分析之后检查 C-- 程序的符号定义、类型匹配和表达式合法性。输入合法时不输出内容，发现语义错误时输出错误类型、行号与说明，返回非零状态。

## 构建与运行

只运行本实验时，进入自身的 `Code` 目录，依次执行 `make` 和 `make test`；无需构建其他实验，也无需项目外层的测试文件。

以下命令均在项目根目录 **`NJU_C_Compiler`** 下执行。需要 GCC、GNU Make、Flex、Bison 和 m4；自动测试另外需要 Python 3.8 以上版本。

```bash
sudo apt-get install build-essential flex bison m4 python3
make -C lab2/Code
./lab2/Code/parser lab2/test/invalid/undefined_variable.cmm
```

程序只接受输入文件路径；`invalid/undefined_variable.cmm` 中第 3 行使用了未定义变量 `missing_total`，因此应输出：

```text
Error type 1 at Line 3: Undefined variable 'missing_total'.
```

也可自行创建合法输入，例如 `int main() { return 0; }`。合法输入返回 `0` 且无输出；词法、语法、语义或文件处理失败返回 `1`。诊断写入标准输出，参数和文件错误写入标准错误。

## 检查范围

| 错误类型 | 含义 |
| --- | --- |
| 1–4 | 未定义变量、未定义函数、重复变量或结构体名冲突、重复函数 |
| 5–9 | 赋值类型不匹配、左侧不是左值、非法操作数、返回类型不匹配、调用参数不匹配 |
| 10–14 | 对非数组下标访问、调用普通变量、下标不是整数、对非结构体取域、访问不存在的域 |
| 15–17 | 结构体字段重复或初始化、结构体名字冲突、使用未定义的结构体 |

支持结构等价扩展：结构体按字段类型和顺序比较，忽略字段名；数组按元素类型和维数比较，忽略长度。变量采用实验默认的全局唯一命名规则。函数声明及嵌套作用域选做扩展未实现，不属于当前合法测试输入的范围。

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
| `Makefile` | 生成、独立编译并链接词法与语法分析器，提供测试和清理入口 |

生成的 `parser.tab.c/.h`、`lexer.yy.c`、`parser.output`、`*.o` 和 `*.d` 统一放在 **`Code/build/`**，可执行程序保留为 **`Code/parser`**。例如 `build/main.d` 是 GCC 生成的依赖记录，不是 C 源码；它帮助 Make 在头文件变化后重新编译有关模块。

项目的 `.gitignore` 忽略 `build/`、可执行程序、缓存及 `Code` 内的临时 IR/汇编输出；`test` 中的 `.ir` 等样例仍可提交。需要删除构建产物时，执行 `make -C lab2/Code clean`。生成文件应通过修改 `.l`、`.y` 后重新构建，不直接编辑。

## 阅读代码

建议先读 `main.c` 和头文件了解阶段接口，再读 `symbol_table.c`、`semantic.c`，最后阅读本实验新增模块。语法树使用 `first_child`、`next`、`parent` 表示首个孩子、下一个兄弟和父节点。`FindSyntaxChild()` 按语法单元名查找直接孩子。

语法树节点拥有词素副本，符号表和 IR 可借用这些字符串。因此驱动程序在所有输出完成后调用 `ReleaseSyntaxTree()`；它沿独立分配链回收节点，也覆盖语法错误恢复时丢弃的节点。其他类型和 IR 对象仍按单次编译进程分配，不应把当前接口直接当作可重复调用的常驻编译服务。

## 测试

```bash
make -C lab2/Code test
```

当前运行 76 项检查，包括 `test/` 中重新编写的 20 个具名用例、公共前端诊断、语义错误类型 1–17、空类型传播和条件类型，以及 `Manual/Tests_2_春季班.zip` 中 A/B/C 组共 24 个课程程序。课程预期诊断保存在 `test/official_semantic.json`，按错误类型和行号核对；合法样例要求没有诊断。附件缺失时会明确提示跳过课程样例。

`test/valid` 包含函数调用、结构体数组和浮点类型检查；`test/invalid` 按错误类型 1–17 各保留一个用例。`test/cases.json` 精确规定退出码、错误类型和行号，详细运行方法见 [测试说明](test/README.md)。错误用例返回 `1` 是预期结果。

## 实验资料

- [实验要求](Manual/实验二_C--语义分析.pdf)
- [实验指导](Manual/实验二指导.pdf)
- [课程测试说明](Manual/Tests.pdf)
- [共享文法](../lab1/Manual/C--文法-附录A.pdf)
- [项目说明](../README.md)
