#ifndef NJU_C_COMPILER_LAB3_CODE_SEMANTIC_H_
#define NJU_C_COMPILER_LAB3_CODE_SEMANTIC_H_

#include "symbol_table.h"
#include "syntax_tree.h"

/**
 * @brief 检查整个程序的符号定义与类型。
 * @return 成功返回 0；出错返回非零，错误类型 1–17 的诊断输出到 stdout。
 */
int AnalyzeProgram(SyntaxNode* program);
/* Lab3/4 在分析前注册 read() 和 write(int)，Lab2 不预设这两个函数。 */
void DefineBuiltinFunctions(void);
/* IR 翻译复用已建立的符号类型；size 以字节为单位，NULL 类型大小为 0。 */
Type* AnalyzeExpressionType(SyntaxNode* node);
int GetTypeSize(Type* type);

#endif  // NJU_C_COMPILER_LAB3_CODE_SEMANTIC_H_
