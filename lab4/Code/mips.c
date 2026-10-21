/* 每个函数先扫描 IR 确定栈帧大小；$s8 保存帧入口，$sp 在调用后恢复。 */
#include "mips.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "symbol_table.h"

typedef struct StackOffsetRecord StackOffsetRecord;
struct StackOffsetRecord {
    char* name;
    int cnt;
    StackOffsetRecord* next;
    StackOffsetRecord* hash_next;
};

static MipsInstruction* CreateMipsInstruction(const char* fmt, ...);
static MipsList* CreateMipsList(void);
static MipsList* CreateMipsRuntime(void);
static void AppendMipsInstruction(MipsList* codes, MipsInstruction* code);
static int GetStackOffset(char* name);
static void AddStackVariable(char* name);
static void PrepareStackFrame(MipsList* mc, IrInstruction* ic);
static void AddStackAggregate(char* name, int size);
static void ClearStackRecords(void);

#define NJU_C_COMPILER_EMIT(...) AppendMipsInstruction(mc, CreateMipsInstruction(__VA_ARGS__))

static MipsInstruction* CreateMipsInstruction(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int length = vsnprintf(NULL, 0, fmt, args);
    if (length < 0) {
        va_end(args);
        return NULL;
    }

    MipsInstruction* node = malloc(sizeof(MipsInstruction));
    node->code = malloc(length + 1);

    va_end(args);
    va_start(args, fmt);
    vsnprintf(node->code, length + 1, fmt, args);
    va_end(args);

    node->next = NULL;
    node->prev = NULL;

    return node;
}
static MipsList* CreateMipsList(void) {
    MipsList* node = malloc(sizeof(MipsList));
    node->head = node->tail = NULL;
    return node;
}
static MipsList* CreateMipsRuntime(void) {
    MipsList* mc = CreateMipsList();
    char* prompt = CreateIrLabel();
    char* newline = CreateIrLabel();
    NJU_C_COMPILER_EMIT(".data");
    NJU_C_COMPILER_EMIT("%s: .asciiz \"Enter an integer:\"", prompt);
    NJU_C_COMPILER_EMIT("%s: .asciiz \"\\n\"", newline);
    NJU_C_COMPILER_EMIT(".globl main");
    NJU_C_COMPILER_EMIT(".text");
    NJU_C_COMPILER_EMIT("read:");
    NJU_C_COMPILER_EMIT("  li $v0, 4");
    NJU_C_COMPILER_EMIT("  la $a0, %s", prompt);
    NJU_C_COMPILER_EMIT("  syscall");
    NJU_C_COMPILER_EMIT("  li $v0, 5");
    NJU_C_COMPILER_EMIT("  syscall");
    NJU_C_COMPILER_EMIT("  jr $ra");
    NJU_C_COMPILER_EMIT("write:");
    NJU_C_COMPILER_EMIT("  li $v0, 1");
    NJU_C_COMPILER_EMIT("  syscall");
    NJU_C_COMPILER_EMIT("  li $v0, 4");
    NJU_C_COMPILER_EMIT("  la $a0, %s", newline);
    NJU_C_COMPILER_EMIT("  syscall");
    NJU_C_COMPILER_EMIT("  move $v0, $0");
    NJU_C_COMPILER_EMIT("  jr $ra");
    return mc;
}
static void AppendMipsInstruction(MipsList* codes, MipsInstruction* code) {
    if (codes->head == NULL) {
        codes->head = codes->tail = code;
        return;
    }
    codes->tail->next = code;
    code->prev = codes->tail;
    codes->tail = code;
}

static int g_parameter_count;
static int g_argument_count;
static int g_stack_argument_count;

MipsList* TranslateIrToMips(IrList* ics) {
    MipsList* mc = CreateMipsRuntime();
    IrInstruction* ic = ics->head;
    while (ic != NULL) {
        switch (ic->kind) {
            case kIrLabel: {
                NJU_C_COMPILER_EMIT("%s:", ic->dest);
            } break;
            case kIrFunction: {
                NJU_C_COMPILER_EMIT("%s:", ic->dest);
                ClearStackRecords();
                PrepareStackFrame(mc, ic);
                g_parameter_count = 0;
                g_argument_count = 0;
                g_stack_argument_count = 0;
            } break;
            case kIrAssign: {
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                }
                int x_offset = GetStackOffset(ic->dest);
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrAdd: {
                int x_offset = GetStackOffset(ic->dest);
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                }
                if (ic->arg2[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t1, %s", ic->arg2 + 1);
                } else {
                    int z_offset = GetStackOffset(ic->arg2);
                    NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", z_offset);
                }
                NJU_C_COMPILER_EMIT("  addu $t0, $t0, $t1");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrSubtract: {
                int x_offset = GetStackOffset(ic->dest);
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                }
                if (ic->arg2[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t1, %s", ic->arg2 + 1);
                } else {
                    int z_offset = GetStackOffset(ic->arg2);
                    NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", z_offset);
                }
                NJU_C_COMPILER_EMIT("  subu $t0, $t0, $t1");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrMultiply: {
                int x_offset = GetStackOffset(ic->dest);
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                }
                if (ic->arg2[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t1, %s", ic->arg2 + 1);
                } else {
                    int z_offset = GetStackOffset(ic->arg2);
                    NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", z_offset);
                }
                NJU_C_COMPILER_EMIT("  mul $t0, $t0, $t1");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrDivide: {
                int x_offset = GetStackOffset(ic->dest);
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                }
                if (ic->arg2[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t1, %s", ic->arg2 + 1);
                } else {
                    int z_offset = GetStackOffset(ic->arg2);
                    NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", z_offset);
                }
                NJU_C_COMPILER_EMIT("  div $t0, $t1");
                NJU_C_COMPILER_EMIT("  mflo $t0");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrAddressOf: {
                int x_offset = GetStackOffset(ic->dest);
                int y_offset = GetStackOffset(ic->arg1);
                NJU_C_COMPILER_EMIT("  la $t0, %d($sp)", y_offset);
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrLoad: {
                int x_offset = GetStackOffset(ic->dest);
                int y_offset = GetStackOffset(ic->arg1);
                NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", y_offset);
                NJU_C_COMPILER_EMIT("  lw $t0, 0($t0)");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrStore: {
                int x_offset = GetStackOffset(ic->dest);
                int y_offset = GetStackOffset(ic->arg1);
                NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", x_offset);
                NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", y_offset);
                NJU_C_COMPILER_EMIT("  sw $t1, 0($t0)");
            } break;
            case kIrGoto: {
                NJU_C_COMPILER_EMIT("  j %s", ic->dest);
            } break;
            case kIrBranchIf: {
                if (ic->arg1[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t0, %s", ic->arg1 + 1);
                } else {
                    int x_offset = GetStackOffset(ic->arg1);
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", x_offset);
                }
                if (ic->arg2[0] == '#') {
                    NJU_C_COMPILER_EMIT("  li $t1, %s", ic->arg2 + 1);
                } else {
                    int y_offset = GetStackOffset(ic->arg2);
                    NJU_C_COMPILER_EMIT("  lw $t1, %d($sp)", y_offset);
                }
                switch (ic->relop->kind) {
                    case kRelLess:
                        NJU_C_COMPILER_EMIT("  blt $t0, $t1, %s", ic->dest);
                        break;
                    case kRelGreater:
                        NJU_C_COMPILER_EMIT("  bgt $t0, $t1, %s", ic->dest);
                        break;
                    case kRelLessEqual:
                        NJU_C_COMPILER_EMIT("  ble $t0, $t1, %s", ic->dest);
                        break;
                    case kRelGreaterEqual:
                        NJU_C_COMPILER_EMIT("  bge $t0, $t1, %s", ic->dest);
                        break;
                    case kRelEqual:
                        NJU_C_COMPILER_EMIT("  beq $t0, $t1, %s", ic->dest);
                        break;
                    case kRelNotEqual:
                        NJU_C_COMPILER_EMIT("  bne $t0, $t1, %s", ic->dest);
                        break;
                    case kRelInvalid:
                    default:
                        fprintf(stderr, "Illegal relop kind %d in IFGOTO.\n", ic->relop->kind);
                        exit(1);
                }
            } break;
            case kIrReturn: {
                int x_offset = GetStackOffset(ic->dest);
                NJU_C_COMPILER_EMIT("  lw $v0, %d($sp)", x_offset);
                NJU_C_COMPILER_EMIT("  move $sp, $s8");
                NJU_C_COMPILER_EMIT("  lw $s8, -4($s8)");
                NJU_C_COMPILER_EMIT("  jr $ra");
            } break;
            case kIrAllocate: {
                ;
            } break;
            /* 实参在 IR 中逆序排列：前四个进入 $a0–$a3，其余压入调用者栈。 */
            case kIrArgument: {
                if (g_argument_count == 0) {
                    IrInstruction* tic = ic;
                    while (tic && tic->kind == kIrArgument) {
                        g_argument_count += 1;
                        tic = tic->next;
                    }
                    g_stack_argument_count = 0;
                }
                int x_offset = GetStackOffset(ic->dest);
                NJU_C_COMPILER_EMIT("  lw $t0, %d($sp)", x_offset);
                if (g_argument_count >= 5) {
                    g_stack_argument_count += 1;
                    NJU_C_COMPILER_EMIT("  sw $t0, -%d($sp)", g_stack_argument_count << 2);
                } else {
                    NJU_C_COMPILER_EMIT("  move $a%d, $t0", g_argument_count - 1);
                }
                g_argument_count -= 1;
            } break;
            /* 调用者保存 $ra；被调用者恢复 $s8 和 $sp，再将结果留在 $v0。 */
            case kIrCall: {
                assert(g_argument_count == 0);
                int x_offset = GetStackOffset(ic->dest);
                if (g_stack_argument_count > 0) {
                    NJU_C_COMPILER_EMIT("  addi $sp, $sp, -%d", g_stack_argument_count << 2);
                }
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, -4");
                NJU_C_COMPILER_EMIT("  sw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  jal %s", ic->arg1);
                NJU_C_COMPILER_EMIT("  lw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, 4");
                if (g_stack_argument_count > 0) {
                    NJU_C_COMPILER_EMIT("  addi $sp, $sp, %d", g_stack_argument_count << 2);
                    g_stack_argument_count = 0;
                }
                NJU_C_COMPILER_EMIT("  move $t0, $v0");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrParameter: {
                g_parameter_count += 1;
                int x_offset = GetStackOffset(ic->dest);
                if (g_parameter_count <= 4) {
                    NJU_C_COMPILER_EMIT("  move $t0, $a%d", g_parameter_count - 1);
                } else {
                    NJU_C_COMPILER_EMIT("  lw $t0, %d($s8)", ((g_parameter_count - 5) << 2) + 4);
                }
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrRead: {
                int x_offset = GetStackOffset(ic->dest);
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, -4");
                NJU_C_COMPILER_EMIT("  sw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  jal read");
                NJU_C_COMPILER_EMIT("  lw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, 4");
                NJU_C_COMPILER_EMIT("  move $t0, $v0");
                NJU_C_COMPILER_EMIT("  sw $t0, %d($sp)", x_offset);
            } break;
            case kIrWrite: {
                int x_offset = GetStackOffset(ic->dest);
                NJU_C_COMPILER_EMIT("  lw $a0, %d($sp)", x_offset);
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, -4");
                NJU_C_COMPILER_EMIT("  sw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  jal write");
                NJU_C_COMPILER_EMIT("  lw $ra, 0($sp)");
                NJU_C_COMPILER_EMIT("  addi $sp, $sp, 4");
                NJU_C_COMPILER_EMIT("  move $t0, $v0");
            } break;
            default:
                fprintf(stderr, "Unrecognized IC kind: %d\n", ic->kind);
                exit(EXIT_FAILURE);
        }
        ic = ic->next;
    }
    return mc;
}

void PrintMips(FILE* f, MipsList* mcs) {
    MipsInstruction* mc = mcs->head;
    while (mc != NULL) {
        fprintf(f, "%s\n", mc->code);
        mc = mc->next;
    }
}

/* 栈帧内已分配的 4 字节槽数量；偏移均相对于固定的局部变量区。 */
static int g_frame_slot_count;
static StackOffsetRecord* g_stack_records_head;
static StackOffsetRecord* g_stack_records_tail;
static StackOffsetRecord* g_stack_table[kSymbolTableBucketCount];

static StackOffsetRecord* FindStackRecord(char* name) {
    for (StackOffsetRecord* cur = g_stack_table[HashSymbolName(name)]; cur; cur = cur->hash_next) {
        if (!strcmp(cur->name, name))
            return cur;
    }
    return NULL;
}
static void AppendStackRecord(StackOffsetRecord* cur) {
    unsigned int slot = HashSymbolName(cur->name);
    cur->hash_next = g_stack_table[slot];
    g_stack_table[slot] = cur;
    if (g_stack_records_head == NULL) {
        g_stack_records_head = g_stack_records_tail = cur;
    } else {
        g_stack_records_tail->next = cur;
        g_stack_records_tail = cur;
    }
}
static int GetStackOffset(char* name) {
    StackOffsetRecord* rec = FindStackRecord(name);
    assert(rec != NULL);
    return ((g_frame_slot_count - rec->cnt) << 2);
}
static void AddStackVariable(char* name) {
    StackOffsetRecord* rec = FindStackRecord(name);
    if (rec == NULL) {
        g_frame_slot_count++;
        StackOffsetRecord* cur = malloc(sizeof(StackOffsetRecord));
        cur->name = malloc(strlen(name) + 1);
        strcpy(cur->name, name);
        cur->cnt = g_frame_slot_count;
        cur->next = NULL;
        AppendStackRecord(cur);
    }
}
static void PrepareStackFrame(MipsList* mc, IrInstruction* ic) {
    ic = ic->next;
    while (ic && ic->kind != kIrFunction) {
        if (ic->kind == kIrAllocate) {
            AddStackAggregate(ic->dest, ic->size);
        } else if (ic->kind != kIrLabel && ic->kind != kIrGoto) {
            if (ic->kind != kIrBranchIf && ic->dest != NULL && ic->dest[0] != '#') {
                AddStackVariable(ic->dest);
            }
            if (ic->kind != kIrCall && ic->arg1 != NULL && ic->arg1[0] != '#') {
                AddStackVariable(ic->arg1);
            }
            if (ic->arg2 != NULL && ic->arg2[0] != '#') {
                AddStackVariable(ic->arg2);
            }
        }
        ic = ic->next;
    }
    NJU_C_COMPILER_EMIT("  addi $sp, $sp, -4");
    NJU_C_COMPILER_EMIT("  sw $s8, 0($sp)");
    NJU_C_COMPILER_EMIT("  la $s8, 4($sp)");
    /* 大栈帧不能直接使用超出 16 位的 addiu 立即数。 */
    int frame_size = g_frame_slot_count << 2;
    if (frame_size <= 32768) {
        NJU_C_COMPILER_EMIT("  addiu $sp, $sp, -%d", frame_size);
    } else {
        NJU_C_COMPILER_EMIT("  li $t8, %d", frame_size);
        NJU_C_COMPILER_EMIT("  subu $sp, $sp, $t8");
    }
}
static void AddStackAggregate(char* name, int size) {
    g_frame_slot_count += (size >> 2);
    StackOffsetRecord* cur = malloc(sizeof(StackOffsetRecord));
    cur->name = malloc(strlen(name) + 1);
    strcpy(cur->name, name);
    cur->cnt = g_frame_slot_count;
    cur->next = NULL;
    AppendStackRecord(cur);
}
static void ClearStackRecords(void) {
    g_frame_slot_count = 0;
    StackOffsetRecord* cur = g_stack_records_head;
    while (cur != NULL) {
        StackOffsetRecord* next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    g_stack_records_head = g_stack_records_tail = NULL;
    memset(g_stack_table, 0, sizeof(g_stack_table));
}

#undef NJU_C_COMPILER_EMIT
