#ifndef NJU_C_COMPILER_LAB3_CODE_IR_H_
#define NJU_C_COMPILER_LAB3_CODE_IR_H_

#include <stdbool.h>
#include <stdio.h>

typedef struct IrList IrList;
typedef struct IrInstruction IrInstruction;
typedef struct IrRelation IrRelation;

struct IrList {
    IrInstruction* head;
    IrInstruction* tail;
};

struct IrInstruction {
    enum {
        kIrLabel,
        kIrFunction,
        kIrAssign,
        kIrAdd,
        kIrSubtract,
        kIrMultiply,
        kIrDivide,
        kIrAddressOf,
        kIrLoad,
        kIrStore,
        kIrGoto,
        kIrBranchIf,
        kIrReturn,
        kIrAllocate,
        kIrArgument,
        kIrCall,
        kIrParameter,
        kIrRead,
        kIrWrite
    } kind;
    char* arg1;
    char* arg2;
    char* dest;
    // For kIrFunction, dest <- f
    // For kIrBranchIf, arg1 <- x, arg2 <- y, dest <- z
    // For kIrCall, dest <- x, arg1 <- f
    // For other instructions, dest <- x, arg1 <- y and arg2 <- z
    int size;           // For kIrAllocate
    IrRelation* relop;  // For kIrBranchIf

    IrInstruction* next;
    IrInstruction* prev;
};

struct IrRelation {
    enum {
        kRelLess,
        kRelGreater,
        kRelLessEqual,
        kRelGreaterEqual,
        kRelEqual,
        kRelNotEqual,
        kRelInvalid
    } kind;
};

/* 生成的名字避开全部源程序符号，防止与用户变量或函数冲突。 */
char* CreateIrConstant(int value);

char* CreateIrTemporary(void);
char* CreateIrLabel(void);

/* 指令链表拥有链接关系；操作数字符串在本次编译期间保持有效。 */
IrInstruction* CreateIrInstruction(int kind, ...);
IrList* CreateIrList(void);
IrList* WrapIrInstruction(IrInstruction* code);
void AppendIrInstruction(IrList* target, IrInstruction* cur);
void AppendIrList(IrList* target, IrList* cur);

/* 打印课程规定的线性三地址代码，不修改指令链。 */
void PrintIrInstruction(FILE* f, IrInstruction* code);
void PrintIr(FILE* f, IrList* codes);

/* 聚合类型形参已经保存地址；局部聚合变量需要先取地址。 */
void RegisterIrParameter(char* name);
bool IsIrParameter(char* name);

#endif  // NJU_C_COMPILER_LAB3_CODE_IR_H_
