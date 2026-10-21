# Lab2 测试用例

本目录包含 20 个按功能编写的 C-- 源程序，共执行 20 项检查。`valid/` 保存本阶段应接受的程序，`invalid/` 保存预期报错的程序；所有文件均在 `cases.json` 中登记。

以下命令在项目根目录执行：

```bash
make -C lab2/Code
python3 lab2/test/run_tests.py --fixtures-only
make -C lab2/Code test  # 运行本实验的全部检查
```

测试脚本将编译结果写入临时目录，不修改本目录。错误用例返回 `1` 是预期结果。

## 用例与预期

| 文件 | 检查目标 | 预期 |
| --- | --- | --- |
| [function_call.cmm](valid/function_call.cmm) | 合法的参数与返回类型 | 返回 0，无诊断 |
| [struct_array.cmm](valid/struct_array.cmm) | 结构体字段内的数组 | 返回 0，无诊断 |
| [float_calculation.cmm](valid/float_calculation.cmm) | 浮点函数参数、运算与赋值 | 返回 0，无诊断 |
| [undefined_variable.cmm](invalid/undefined_variable.cmm) | 未定义变量 | 类型 1，第 3 行 |
| [undefined_function.cmm](invalid/undefined_function.cmm) | 未定义函数 | 类型 2，第 3 行 |
| [duplicate_variable.cmm](invalid/duplicate_variable.cmm) | 重复变量定义 | 类型 3，第 4 行 |
| [duplicate_function.cmm](invalid/duplicate_function.cmm) | 重复函数定义 | 类型 4，第 2 行 |
| [assignment_type.cmm](invalid/assignment_type.cmm) | 赋值两侧类型不匹配 | 类型 5，第 4 行 |
| [non_lvalue.cmm](invalid/non_lvalue.cmm) | 向非左值表达式赋值 | 类型 6，第 3 行 |
| [operand_type.cmm](invalid/operand_type.cmm) | 运算数类型不匹配 | 类型 7，第 4 行 |
| [return_type.cmm](invalid/return_type.cmm) | 返回值类型不匹配 | 类型 8，第 3 行 |
| [argument_type.cmm](invalid/argument_type.cmm) | 调用参数类型不匹配 | 类型 9，第 8 行 |
| [not_an_array.cmm](invalid/not_an_array.cmm) | 对普通变量使用下标 | 类型 10，第 4 行 |
| [not_a_function.cmm](invalid/not_a_function.cmm) | 调用普通变量 | 类型 11，第 4 行 |
| [index_type.cmm](invalid/index_type.cmm) | 下标类型不是整数 | 类型 12，第 4 行 |
| [not_a_struct.cmm](invalid/not_a_struct.cmm) | 对普通变量取域 | 类型 13，第 4 行 |
| [undefined_field.cmm](invalid/undefined_field.cmm) | 访问未定义字段 | 类型 14，第 6 行 |
| [duplicate_field.cmm](invalid/duplicate_field.cmm) | 重复结构体字段 | 类型 15，第 4 行 |
| [duplicate_struct.cmm](invalid/duplicate_struct.cmm) | 重复结构体名 | 类型 16，第 2 行 |
| [undefined_struct.cmm](invalid/undefined_struct.cmm) | 使用未定义结构体 | 类型 17，第 3 行 |

## 维护预期结果

`cases.json` 中每条记录对应一个 `.cmm` 文件；漏登记、新增未登记文件或重复登记都会使测试失败。诊断用例的 `exit_code` 和 `errors` 分别表示退出码及按顺序排列的 `[错误类型, 行号]`。

预期结果按语言规则和手算结果维护。更改程序时，应同步检查清单中的行号和输入输出。

测试运行器只读取本实验目录中的文件。
