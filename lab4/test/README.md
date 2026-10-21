# Lab4 测试用例

本目录包含 10 个按功能编写的 C-- 源程序，共执行 15 项检查。`valid/` 保存本阶段应接受的程序，`invalid/` 保存预期报错的程序；所有文件均在 `cases.json` 中登记。

以下命令在项目根目录执行：

```bash
make -C lab4/Code
python3 lab4/test/run_tests.py --fixtures-only --require-spim
make -C lab4/Code test  # 运行本实验的全部检查
```

测试脚本将编译结果写入临时目录，不修改本目录。错误用例返回 `1` 是预期结果。

本阶段运行输出由真实 SPIM 验证。`--require-spim` 在找不到模拟器时失败；普通回归会提示跳过运行检查。可用 `SPIM` 环境变量指定模拟器及参数。

## 用例与预期

| 文件 | 检查目标 | 预期 |
| --- | --- | --- |
| [running_total.cmm](valid/running_total.cmm) | 多次输入、循环及有符号累加 | 输入 `[4, 8, -3, 6, 2]` → 输出 `[13]`；输入 `[0]` → 输出 `[0]`；输入 `[2, -5, -7]` → 输出 `[-12]` |
| [recursive_fibonacci.cmm](valid/recursive_fibonacci.cmm) | 双递归调用与调用者栈恢复 | 输入 `[0]` → 输出 `[0]`；输入 `[1]` → 输出 `[1]`；输入 `[10]` → 输出 `[55]` |
| [eight_argument_calls.cmm](valid/eight_argument_calls.cmm) | 寄存器及栈上传参、连续调用恢复 | 输入 `[]` → 输出 `[324]` |
| [large_stack_frame.cmm](valid/large_stack_frame.cmm) | 超出 16 位范围的栈偏移 | 输入 `[]` → 输出 `[45]` |
| [array_reference.cmm](valid/array_reference.cmm) | 数组传参后写回调用者 | 输入 `[]` → 输出 `[26, 7]` |
| [signed_comparisons.cmm](valid/signed_comparisons.cmm) | 有符号比较及负数除法截断 | 输入 `[-8, 2]` → 输出 `[1, 0, 2]`；输入 `[9, 9]` → 输出 `[0, 1, -3]` |
| [nested_loop_matrix.cmm](valid/nested_loop_matrix.cmm) | 嵌套循环和二维数组 | 输入 `[]` → 输出 `[10, 11, 12, 20, 21, 22]` |
| [independent_struct_copy.cmm](valid/independent_struct_copy.cmm) | 修改源结构体后副本保持不变 | 输入 `[]` → 输出 `[12]` |
| [argument_count.cmm](invalid/argument_count.cmm) | 实参数量不匹配 | 类型 9，第 8 行 |
| [invalid_character.cmm](invalid/invalid_character.cmm) | 非法字符诊断 | 类型 A，第 3 行 |

## 维护预期结果

`cases.json` 中每条记录对应一个 `.cmm` 文件；漏登记、新增未登记文件或重复登记都会使测试失败。诊断用例的 `exit_code` 和 `errors` 分别表示退出码及按顺序排列的 `[错误类型, 行号]`。运行用例的 `runs` 列出整数输入和按顺序期待的 `write()` 输出。

预期结果按语言规则和手算结果维护。更改程序时，应同步检查清单中的行号和输入输出。

测试运行器只读取本实验目录中的文件。
