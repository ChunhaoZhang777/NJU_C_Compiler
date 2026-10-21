# NJU C 编译器实验

本项目包含 C-- 编译器的四个实验阶段。每个 Lab 都保留完整的阶段实现，可独立构建、运行和测试，不依赖其他 Lab 的源码或构建产物。

| 实验 | 内容 | 说明 |
| --- | --- | --- |
| Lab1 | 词法分析、语法分析与语法树输出 | [实验说明](lab1/README.md) |
| Lab2 | 语义分析、符号表与类型检查 | [实验说明](lab2/README.md) |
| Lab3 | 三地址中间代码生成 | [实验说明](lab3/README.md) |
| Lab4 | MIPS32 目标代码生成 | [实验说明](lab4/README.md) |

## 目录结构

```text
NJU_C_Compiler/
├── README.md
├── lab1/
├── lab2/
├── lab3/
└── lab4/
```

每个 Lab 内包含 `Code/`（源码与 Makefile）、`test/`（测试用例、预期结果与运行脚本）、`Manual/`（实验资料）及 `README.md`。

## 使用方式

构建需要 GCC、GNU Make、Flex、Bison 和 m4；自动测试需要 Python 3.8 以上版本，Lab4 的汇编运行还需要 SPIM。具体安装、运行参数和测试说明见各实验 README。

完成哪个实验，就进入它的 `Code` 目录构建和测试。例如：

```bash
cd lab1/Code
make
make test
make clean
```

Lab2–4 使用各自目录中的相同命令；Lab4 另外支持 `make test-mips`，要求使用真实 SPIM 验证运行结果。

参考实现来源：[IAmDannyCN/NJU_Compiler](https://github.com/IAmDannyCN/NJU_Compiler)。共享文法见 [C-- 文法附录 A](lab1/Manual/C--文法-附录A.pdf)。
