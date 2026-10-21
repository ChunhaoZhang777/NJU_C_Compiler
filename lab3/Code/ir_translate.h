#ifndef NJU_C_COMPILER_LAB3_CODE_IR_TRANSLATE_H_
#define NJU_C_COMPILER_LAB3_CODE_IR_TRANSLATE_H_

#include "ir.h"
#include "syntax_tree.h"

/**
 * @brief 为已经通过语义检查的程序生成三地址代码。
 * @return 指令链表；操作数借用的词素在打印完成前必须保持有效。
 */
IrList* TranslateProgramToIr(SyntaxNode* program);

#endif  // NJU_C_COMPILER_LAB3_CODE_IR_TRANSLATE_H_
