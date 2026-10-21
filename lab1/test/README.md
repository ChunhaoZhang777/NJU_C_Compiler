# Lab1 测试用例

本目录包含 12 个按功能编写的 C-- 源程序，共执行 12 项检查。`valid/` 保存本阶段应接受的程序，`invalid/` 保存预期报错的程序；所有文件均在 `cases.json` 中登记。

以下命令在项目根目录执行：

```bash
make -C lab1/Code
python3 lab1/test/run_tests.py --fixtures-only
make -C lab1/Code test  # 运行本实验的全部检查
```

测试脚本将编译结果写入临时目录，不修改本目录。错误用例返回 `1` 是预期结果。

## 用例与预期

| 文件 | 检查目标 | 预期 |
| --- | --- | --- |
| [minimal_program.cmm](valid/minimal_program.cmm) | 最小程序与语法树入口 | 返回 0，输出语法树 |
| [numeric_literals.cmm](valid/numeric_literals.cmm) | 十进制、八进制、十六进制、指数浮点数 | 返回 0，输出语法树 |
| [comments_between_tokens.cmm](valid/comments_between_tokens.cmm) | 注释与词法单元行号 | 返回 0，输出语法树 |
| [dangling_else.cmm](valid/dangling_else.cmm) | 嵌套 if/else 的语法匹配 | 返回 0，输出语法树 |
| [struct_and_matrix.cmm](valid/struct_and_matrix.cmm) | 结构体声明和二维下标 | 返回 0，输出语法树 |
| [expression_precedence.cmm](valid/expression_precedence.cmm) | 算术、逻辑与一元运算的语法 | 返回 0，输出语法树 |
| [semantic_errors_allowed.cmm](valid/semantic_errors_allowed.cmm) | 词法/语法合法但语义未定义的标识符 | 返回 0，输出语法树 |
| [invalid_character.cmm](invalid/invalid_character.cmm) | 非法字符诊断 | 类型 A，第 3 行 |
| [bad_octal.cmm](invalid/bad_octal.cmm) | 非法八进制数字 | 类型 A，第 3 行 |
| [unfinished_comment.cmm](invalid/unfinished_comment.cmm) | 未闭合块注释的起始行 | 类型 A，第 5 行 |
| [missing_semicolon.cmm](invalid/missing_semicolon.cmm) | 缺少分号时的语法错误行号 | 类型 B，第 4 行 |
| [missing_parenthesis.cmm](invalid/missing_parenthesis.cmm) | 缺少右括号 | 类型 B，第 3 行 |

## 维护预期结果

`cases.json` 中每条记录对应一个 `.cmm` 文件；漏登记、新增未登记文件或重复登记都会使测试失败。诊断用例的 `exit_code` 和 `errors` 分别表示退出码及按顺序排列的 `[错误类型, 行号]`。`contains` 指定语法树中必须出现的片段。

预期结果按语言规则和手算结果维护。更改程序时，应同步检查清单中的行号和输入输出。
