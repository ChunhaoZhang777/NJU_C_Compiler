#ifndef NJU_C_COMPILER_LAB4_CODE_MIPS_H_
#define NJU_C_COMPILER_LAB4_CODE_MIPS_H_

#include <stdio.h>

#include "ir.h"

typedef struct MipsList MipsList;
typedef struct MipsInstruction MipsInstruction;

struct MipsList {
    MipsInstruction* head;
    MipsInstruction* tail;
};
struct MipsInstruction {
    char* code;
    MipsInstruction* next;
    MipsInstruction* prev;
};

/**
 * @brief 将 IR 翻译成 SPIM 可执行的 MIPS32 汇编。
 * @return 生成的汇编指令链表，PrintMips() 只读取该链表。
 */
MipsList* TranslateIrToMips(IrList* codes);
void PrintMips(FILE* output, MipsList* codes);

#endif  // NJU_C_COMPILER_LAB4_CODE_MIPS_H_
