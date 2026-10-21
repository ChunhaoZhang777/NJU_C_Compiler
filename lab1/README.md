# Lab1 词法分析与语法分析

本实验读取 C-- 源文件，完成词法分析、语法分析，构造并打印语法树。发现词法或语法错误时输出错误类型、行号和说明，并以非零状态退出。

Lab1 只判断源程序的词法和语法是否正确。例如，`test/valid/semantic_errors_allowed.cmm` 使用了未声明的变量，但其语法合法，因此本阶段仍会打印语法树；未定义变量、类型不匹配等问题由 Lab2 检查。

## 目录与文件职责

```text
lab1/
├── Code/                # 手写源码、Makefile 和当前构建产物
│   ├── main.c           # 命令行入口，组织解析过程并决定是否打印语法树
│   ├── frontend.h       # 声明词法、语法模块的接口与错误状态
│   ├── lexer.l          # Flex 词法规则，识别词法单元、跳过注释、报告词法错误
│   ├── parser.y         # Bison 文法、运算符优先级、语法树构造和错误恢复
│   ├── syntax_tree.h    # 语法树节点类型与操作接口
│   ├── syntax_tree.c    # 创建节点、连接子节点、先序打印语法树
│   ├── Makefile         # 生成分析器、编译、测试与清理
│   └── build/           # 自动生成的分析器、目标文件与依赖记录
├── Manual/              # 实验要求、指导与文法
├── test/                # 12 个具名用例、预期结果清单和运行说明
│   ├── valid/           # 7 个词法和语法合法的源程序
│   ├── invalid/         # 5 个预期报错的源程序
│   ├── cases.json       # 退出码、诊断行号及语法树关键内容
│   └── README.md        # 用例说明和执行方法
└── README.md
```

编译后会在 `Code/build/` 中看到以下自动生成文件，可执行程序为 `Code/parser`：

| 文件 | 用途 | 维护方式 |
| --- | --- | --- |
| `build/lexer.yy.c` | Flex 根据 `lexer.l` 生成的词法分析器 | 修改 `lexer.l` 后重新构建 |
| `build/parser.tab.c/.h` | Bison 生成的语法分析器及词法单元声明 | 修改 `parser.y` 后重新构建 |
| `build/parser.output` | Bison 的文法状态报告，用于排查分析冲突 | 查看即可 |
| `build/*.o` | C 源文件编译后的目标文件 | 自动生成 |
| `build/*.d` | GCC 生成的头文件依赖记录 | 自动生成 |
| `parser` | 最终可执行程序 | 用于运行测试 |

例如 `build/main.d` 记录 `main.c` 和相关头文件的依赖关系，帮助 Make 判断是否需要重新编译。它不是 C 源码；阅读入口程序应打开 [main.c](Code/main.c)。词法和语法生成文件分别编译，通过 `frontend.h` 和生成的声明文件连接。

上述构建产物由项目根目录的 `.gitignore` 忽略。忽略规则不会删除磁盘文件，需要清理时使用 `make -C lab1/Code clean`。旧布局中的 `Code/main.d` 等文件会在清理时删除，新的依赖记录位于 `Code/build/`。

## 构建与运行

只运行本实验时，进入自身的 `Code` 目录，依次执行 `make` 和 `make test`；无需构建其他实验，也无需项目外层的测试文件。

下面的命令均在项目根目录 **`NJU_C_Compiler`** 下执行。需要 GCC、GNU Make、Flex、Bison、m4；运行回归测试还需要 Python 3.8 以上版本。

Ubuntu 可以安装这些依赖：

```bash
sudo apt-get install build-essential flex bison m4 python3
```

仅构建 Lab1，然后运行一个合法样例：

```bash
make -C lab1/Code
./lab1/Code/parser lab1/test/valid/minimal_program.cmm
```

也可以在 `lab1/Code` 中运行 `make`，此时样例的相对路径应写为 `../test/valid/minimal_program.cmm`。

程序只接受一个输入文件路径：

```bash
./lab1/Code/parser path/to/program.cmm
```

语法树和词法、语法错误信息写入标准输出；参数错误和文件读取错误写入标准错误。成功返回 `0`，分析失败或文件无法打开时返回 `1`。需要保存语法树时，可以使用重定向：

```bash
./lab1/Code/parser lab1/test/valid/minimal_program.cmm > /tmp/lab1-tree.txt
```

## 如何理解输出

合法源程序按先序遍历输出语法树。每层缩进两个空格；非终结符显示名称和起始行号；空产生式省略；`ID`、`TYPE`、`INT`、`FLOAT` 额外显示词素或数值。整数统一按十进制打印。

运行 `valid/minimal_program.cmm` 时，输出开头为：

```text
Program (1)
  ExtDefList (1)
    ExtDef (1)
      Specifier (1)
        TYPE: int
      FunDec (1)
        ID: main
        LP
        RP
```

这棵树保留括号、分号等语法单元，是实验要求的语法树，并非省略这些细节的抽象语法树。

有错误时只输出诊断信息。错误类型 `A` 表示词法错误，`B` 表示语法错误。例如：

```bash
./lab1/Code/parser lab1/test/invalid/invalid_character.cmm
```

```text
Error type A at Line 3: Mysterious characters '@'
```

`invalid/missing_semicolon.cmm` 会报告第 4 行的语法错误。语法错误位置通常对应分析器发现无法继续匹配文法的词法单元；例如上一行缺少分号时，可能到下一行才发现错误。

## 测试与清理

只运行 Lab1 的自动回归测试：

```bash
make -C lab1/Code test
```

该命令先构建 Lab1，再运行 `lab1/test/run_tests.py`，当前包含 22 项检查：12 个具名文件用例，以及命令行参数、非法字符、未闭合注释、超过一万行的输入、语法错误、非法数字、文件末尾行注释和合法数字与语法树行号的 10 项基础回归。成功时输出：

```text
Lab1: 22 checks passed
```

`test/valid` 和 `test/invalid` 中的文件按功能命名，覆盖数字进制、注释、嵌套条件、结构体/多维数组、表达式以及常见词法/语法错误。每个文件都登记在 `test/cases.json` 中并参与自动测试，详细预期和执行方法见 [测试说明](test/README.md)。

仅清理 Lab1 的构建产物使用：

```bash
make -C lab1/Code clean
```

该命令保留手写源码和测试样例，下次运行 `make -C lab1/Code` 会重新生成分析器。

## 阅读代码

建议按 `main.c` → `syntax_tree.h` → `lexer.l` → `parser.y` → `syntax_tree.c` 的顺序阅读。`main.c` 调用 `yyparse()`；语法分析器调用 `yylex()` 获取词法单元；文法动作通过 `CreateSyntaxNode()`、`AddSyntaxChildren()` 构造树，最后由 `PrintSyntaxTree()` 打印。

`SyntaxNode` 采用“第一个孩子、下一个兄弟”的表示方式：`first_child` 指向第一个子节点，`next` 指向下一个兄弟，`parent` 指向父节点。`ReleaseSyntaxTree()` 沿独立分配链释放全部节点，包含语法错误恢复时丢弃的节点。

## 实验资料

- [实验要求](Manual/实验一_C--词法分析器与语法分析器的实现.pdf)
- [实验指导](Manual/实验一指导.pdf)
- [C-- 文法附录 A](Manual/C--文法-附录A.pdf)
- [整个项目的构建与实现范围](../README.md)
