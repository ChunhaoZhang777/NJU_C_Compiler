/* 三张独立的链式哈希表保存变量、结构体名与函数，完整名称用于处理冲突。 */
#include "symbol_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct SymbolEntry {
    char* name;
    void* value;
    struct SymbolEntry* next;
} SymbolEntry;
static SymbolEntry* g_struct_table[kSymbolTableBucketCount];
static SymbolEntry* g_symbol_table[kSymbolTableBucketCount];
static SymbolEntry* g_function_table[kSymbolTableBucketCount];

unsigned int HashSymbolName(const char* name) {
    unsigned int value = 0;
    for (; *name; ++name)
        value = value * 137u + (unsigned char)*name;
    return value % kSymbolTableBucketCount;
}

static void* FindSymbolEntry(SymbolEntry** table, const char* name) {
    for (SymbolEntry* cur = table[HashSymbolName(name)]; cur; cur = cur->next)
        if (!strcmp(cur->name, name))
            return cur->value;
    return NULL;
}

static void InsertSymbolEntry(SymbolEntry** table, char* name, void* value) {
    unsigned int slot = HashSymbolName(name);
    assert(FindSymbolEntry(table, name) == NULL);
    SymbolEntry* entry = malloc(sizeof(*entry));
    entry->name = name;
    entry->value = value;
    entry->next = table[slot];
    table[slot] = entry;
}

// CONSTRUCT
// basic type
Type* GetIntType(void) {
    static Type type = {.kind = kBasicType, .basic = 0};
    return &type;
}
Type* GetFloatType(void) {
    static Type type = {.kind = kBasicType, .basic = 1};
    return &type;
}

// array
Type* CreateArrayType(Type* element_type, int size, int elem_size) {
    Type* cur = malloc(sizeof(Type));
    cur->kind = kArrayType;
    cur->size = size;
    cur->elem = element_type;
    cur->elem_size = elem_size;
    return cur;
}

// struct
Type* CreateStructType(void) {
    Type* cur = malloc(sizeof(Type));
    cur->kind = kStructType;
    cur->structure = NULL;
    cur->tail = NULL;
    cur->stru_size = 0;
    return cur;
}
FieldList* CreateField(char* name, Type* type) {
    FieldList* cur = malloc(sizeof(FieldList));
    cur->type = type;
    cur->name = name;
    cur->next = NULL;
    return cur;
}
void AddStructField(Type* struct_type, FieldList* field) {
    if (struct_type->tail == NULL) {
        struct_type->tail = struct_type->structure = field;
    } else {
        struct_type->tail->next = field;
        struct_type->tail = field;
    }
}

// function
Function* CreateFunction(char* name, Type* return_type) {
    Function* cur = malloc(sizeof(Function));
    cur->name = name;
    cur->return_type = return_type;
    cur->argc = 0;
    cur->argv = cur->tail = NULL;
    return cur;
}
FunctionArg* CreateFunctionArgument(Type* type) {
    FunctionArg* cur = malloc(sizeof(FunctionArg));
    cur->type = type;
    cur->next = NULL;
    return cur;
}
void AddFunctionArgument(Function* func, FunctionArg* arg) {
    if (func->tail == NULL) {
        func->argv = arg;
    } else {
        func->tail->next = arg;
    }
    func->tail = arg;
    func->argc += 1;
}

// The tables compare complete identifiers, even when hashes collide.
void InsertStructSymbol(char* name, Type* ty) {
    InsertSymbolEntry(g_struct_table, name, ty);
}
void InsertVariableSymbol(char* name, Type* ty) {
    InsertSymbolEntry(g_symbol_table, name, ty);
}
void InsertFunctionSymbol(Function* func) {
    InsertSymbolEntry(g_function_table, func->name, func);
}
Type* FindVariableSymbol(const char* name) {
    return FindSymbolEntry(g_symbol_table, name);
}
Type* FindStructSymbol(const char* name) {
    return FindSymbolEntry(g_struct_table, name);
}
Function* FindFunctionSymbol(const char* name) {
    return FindSymbolEntry(g_function_table, name);
}

/* 数组比较元素类型及维数；结构体按字段类型与顺序做结构等价比较。 */
bool AreTypesEquivalent(Type* x, Type* y) {
    if (x == NULL || y == NULL) {
        return false;
    }
    if (x->kind != y->kind) {
        return false;
    }
    if (x->kind == kBasicType) {
        return x->basic == y->basic;
    }
    if (x->kind == kArrayType) {
        return AreTypesEquivalent(x->elem, y->elem);
    }
    // x->kind == kStructType
    FieldList* nodex = x->structure;
    FieldList* nodey = y->structure;
    while (true) {
        if (nodex == NULL && nodey == NULL) {
            break;
        }
        if (nodex == NULL || nodey == NULL) {
            return false;
        }
        if (!AreTypesEquivalent(nodex->type, nodey->type)) {
            return false;
        }
        nodex = nodex->next;
        nodey = nodey->next;
    }
    return true;
}
bool AreFunctionArgumentsCompatible(Function* funcx, Function* funcy) {
    if (funcx->argc != funcy->argc) {
        return false;
    }
    FunctionArg* argx = funcx->argv;
    FunctionArg* argy = funcy->argv;
    while (argx != NULL) {
        if (!AreTypesEquivalent(argx->type, argy->type)) {
            return false;
        }
        argx = argx->next;
        argy = argy->next;
    }
    return true;
}
