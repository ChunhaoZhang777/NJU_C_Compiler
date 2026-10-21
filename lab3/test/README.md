# Lab3 测试用例

本目录包含 10 个按功能编写的 C-- 源程序，共执行 12 项检查。`valid/` 保存本阶段应接受的程序，`invalid/` 保存预期报错的程序；所有文件均在 `cases.json` 中登记。

以下命令在项目根目录执行：

```bash
make -C lab3/Code
python3 lab3/test/run_tests.py --fixtures-only
make -C lab3/Code test  # 运行本实验的全部检查
```

测试脚本将编译结果写入临时目录，不修改本目录。错误用例返回 `1` 是预期结果。

运行结果通过 `ir_vm.py` 解释生成的 IR 后核对。直接执行编译器只生成 `.ir` 文件，不会打印程序的 `write()` 输出。

## 用例与预期

| 文件 | 检查目标 | 预期 |
| --- | --- | --- |
| [integer_arithmetic.cmm](valid/integer_arithmetic.cmm) | 整数加减乘除和负数 | 输入 `[]` → 输出 `[36, 7, -11]` |
| [conditional_sum.cmm](valid/conditional_sum.cmm) | 输入边界与循环求和 | 输入 `[0]` → 输出 `[0]`；输入 `[6]` → 输出 `[21]`；输入 `[-3]` → 输出 `[0]` |
| [short_circuit_state.cmm](valid/short_circuit_state.cmm) | 短路求值跳过赋值副作用 | 输入 `[]` → 输出 `[1, 1]` |
| [array_accumulation.cmm](valid/array_accumulation.cmm) | 循环中的数组寻址 | 输入 `[]` → 输出 `[35, 10]` |
| [nested_function_calls.cmm](valid/nested_function_calls.cmm) | 嵌套函数调用的实参和值 | 输入 `[]` → 输出 `[70]` |
| [struct_array_layout.cmm](valid/struct_array_layout.cmm) | 含数组的结构体布局与独立复制 | 输入 `[]` → 输出 `[42, 100]` |
| [multidimensional_offsets.cmm](valid/multidimensional_offsets.cmm) | 三维数组的偏移计算 | 输入 `[]` → 输出 `[27]` |
| [recursive_power.cmm](valid/recursive_power.cmm) | 递归调用与返回结果 | 输入 `[]` → 输出 `[81]` |
| [undefined_operand.cmm](invalid/undefined_operand.cmm) | 未定义操作数阻止生成输出文件 | 类型 1，第 3 行 |
| [missing_semicolon.cmm](invalid/missing_semicolon.cmm) | 缺少分号时的语法错误行号 | 类型 B，第 4 行 |

## 维护预期结果

`cases.json` 中每条记录对应一个 `.cmm` 文件；漏登记、新增未登记文件或重复登记都会使测试失败。诊断用例的 `exit_code` 和 `errors` 分别表示退出码及按顺序排列的 `[错误类型, 行号]`。运行用例的 `runs` 列出整数输入和按顺序期待的 `write()` 输出。

预期结果按语言规则和手算结果维护。更改程序时，应同步检查清单中的行号和输入输出。

测试运行器只读取本实验目录中的文件。
