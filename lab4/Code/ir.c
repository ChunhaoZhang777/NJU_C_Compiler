/* 三地址代码使用双向链表；拼接复用指令节点，操作数字符串在本次编译内有效。 */
#include "ir.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

static int g_temporary_count = 0;
static int g_label_count = 0;

// Store names instead of using hash values as unique identities.
typedef struct ParamEntry {
    char* name;
    struct ParamEntry* next;
} ParamEntry;
static ParamEntry* g_parameter_table[kSymbolTableBucketCount];
void RegisterIrParameter(char* name) {
    if (IsIrParameter(name))
        return;
    unsigned int slot = HashSymbolName(name);
    ParamEntry* entry = malloc(sizeof(*entry));
    entry->name = name;
    entry->next = g_parameter_table[slot];
    g_parameter_table[slot] = entry;
}
bool IsIrParameter(char* name) {
    for (ParamEntry* cur = g_parameter_table[HashSymbolName(name)]; cur; cur = cur->next)
        if (!strcmp(cur->name, name))
            return true;
    return false;
}

// utils
char* CreateIrConstant(int value) {
    int length = snprintf(NULL, 0, "#%d", value);
    char* result = malloc((size_t)length + 1);
    snprintf(result, (size_t)length + 1, "#%d", value);
    return result;
}

// new
// Skip source identifiers so generated operands and labels remain unique.
static char* CreateIrName(char* prefix, int number) {
    int length = snprintf(NULL, 0, "%s%d", prefix, number);
    char* result = malloc((size_t)length + 1);
    snprintf(result, (size_t)length + 1, "%s%d", prefix, number);
    return result;
}
static char* CreateUniqueIrName(char* prefix, int* counter) {
    char* result;
    do {
        result = CreateIrName(prefix, ++*counter);
        if (!FindVariableSymbol(result) && !FindStructSymbol(result) && !FindFunctionSymbol(result))
            return result;
        free(result);
    } while (true);
}
char* CreateIrTemporary(void) {
    return CreateUniqueIrName("_t", &g_temporary_count);
}
char* CreateIrLabel(void) {
    return CreateUniqueIrName("_L", &g_label_count);
}

// IrInstruction
IrInstruction* CreateIrInstruction(int kind, ...) {
    va_list args;
    va_start(args, kind);

    IrInstruction* ic = calloc(1, sizeof(IrInstruction));

    ic->kind = kind;
    ic->arg1 = ic->arg2 = ic->dest = NULL;
    ic->size = 0;
    ic->relop = NULL;

    switch (kind) {
        case kIrAllocate:
            ic->dest = va_arg(args, char*);
            ic->size = va_arg(args, int);
            break;
        case kIrBranchIf:
            ic->arg1 = va_arg(args, char*);
            ic->relop = va_arg(args, IrRelation*);
            ic->arg2 = va_arg(args, char*);
            ic->dest = va_arg(args, char*);
            break;
        case kIrLabel:
        case kIrFunction:
        case kIrGoto:
        case kIrReturn:
        case kIrArgument:
        case kIrParameter:
        case kIrRead:
        case kIrWrite:
            ic->dest = va_arg(args, char*);
            break;
        case kIrAssign:
        case kIrAddressOf:
        case kIrLoad:
        case kIrStore:
        case kIrCall:
            ic->dest = va_arg(args, char*);
            ic->arg1 = va_arg(args, char*);
            break;
        case kIrAdd:
        case kIrSubtract:
        case kIrMultiply:
        case kIrDivide:
            ic->dest = va_arg(args, char*);
            ic->arg1 = va_arg(args, char*);
            ic->arg2 = va_arg(args, char*);
            break;
        default:
            fprintf(stderr, "Invalid IR instruction kind: %d\n", kind);
            exit(EXIT_FAILURE);
    }

    va_end(args);

    return ic;
}
IrList* CreateIrList(void) {
    IrList* ans = malloc(sizeof(IrList));
    ans->head = ans->tail = NULL;
    return ans;
}
IrList* WrapIrInstruction(IrInstruction* code) {
    if (code == NULL) {
        return NULL;
    }
    IrList* codes = malloc(sizeof(IrList));
    assert(code->prev == NULL);
    assert(code->next == NULL);
    codes->head = codes->tail = code;
    return codes;
}
void AppendIrInstruction(IrList* target, IrInstruction* cur) {
    if (target == NULL) {
        target = WrapIrInstruction(cur);
        return;
    }
    if (cur == NULL) {
        return;
    }
    assert(cur->prev == NULL);
    assert(cur->next == NULL);

    if (target->head == NULL) {
        assert(target->tail == NULL);
        target->head = target->tail = cur;
        return;
    }
    assert(target->tail != NULL);
    cur->prev = target->tail;
    target->tail->next = cur;
    target->tail = cur;
}
void AppendIrList(IrList* target, IrList* cur) {
    assert(target != NULL);
    assert(cur != NULL);

    if (cur->head == NULL) {
        assert(cur->tail == NULL);
        return;
    }
    if (target->head == NULL) {
        assert(target->tail == NULL);
        target->head = cur->head;
        target->tail = cur->tail;
        return;
    }
    assert(target->tail != NULL);
    assert(cur->head != NULL);
    target->tail->next = cur->head;
    cur->head->prev = target->tail;
    target->tail = cur->tail;
    return;
}

// IrRelation
//     ans->kind = kind;
//     ans->arg1 = arg1;
//     ans->arg2 = arg2;
//     return ans;

// Output
static void PrintIrRelation(FILE* f, IrRelation* relop) {
    char* rel_str[6] = {"<", ">", "<=", ">=", "==", "!="};
    fprintf(f, "%s", rel_str[relop->kind]);
}
void PrintIrInstruction(FILE* f, IrInstruction* code) {
    switch (code->kind) {
        case kIrAllocate:
            fprintf(f, "DEC %s %d", code->dest, code->size);
            break;
        case kIrBranchIf:
            fprintf(f, "IF %s ", code->arg1);
            PrintIrRelation(f, code->relop);
            fprintf(f, " %s GOTO %s", code->arg2, code->dest);
            break;
        case kIrLabel:
            fprintf(f, "LABEL %s :", code->dest);
            break;
        case kIrFunction:
            fprintf(f, "FUNCTION %s :", code->dest);
            break;
        case kIrGoto:
            fprintf(f, "GOTO %s", code->dest);
            break;
        case kIrReturn:
            fprintf(f, "RETURN %s", code->dest);
            break;
        case kIrArgument:
            fprintf(f, "ARG %s", code->dest);
            break;
        case kIrParameter:
            fprintf(f, "PARAM %s", code->dest);
            break;
        case kIrRead:
            fprintf(f, "READ %s", code->dest);
            break;
        case kIrWrite:
            fprintf(f, "WRITE %s", code->dest);
            break;
        case kIrAssign:
            fprintf(f, "%s := %s", code->dest, code->arg1);
            break;
        case kIrAddressOf:
            fprintf(f, "%s := &%s", code->dest, code->arg1);
            break;
        case kIrLoad:
            fprintf(f, "%s := *%s", code->dest, code->arg1);
            break;
        case kIrStore:
            fprintf(f, "*%s := %s", code->dest, code->arg1);
            break;
        case kIrCall:
            fprintf(f, "%s := CALL %s", code->dest, code->arg1);
            break;
        case kIrAdd:
            fprintf(f, "%s := %s + %s", code->dest, code->arg1, code->arg2);
            break;
        case kIrSubtract:
            fprintf(f, "%s := %s - %s", code->dest, code->arg1, code->arg2);
            break;
        case kIrMultiply:
            fprintf(f, "%s := %s * %s", code->dest, code->arg1, code->arg2);
            break;
        case kIrDivide:
            fprintf(f, "%s := %s / %s", code->dest, code->arg1, code->arg2);
            break;
        default:
            fprintf(stderr, "Invalid IR instruction kind: %d\n", code->kind);
            exit(EXIT_FAILURE);
    }
}
void PrintIr(FILE* f, IrList* codes) {
    if (codes == NULL) {
        return;
    }
    IrInstruction* cur = codes->head;
    while (cur != NULL) {
        PrintIrInstruction(f, cur);
        fprintf(f, "\n");
        cur = cur->next;
    }
}
