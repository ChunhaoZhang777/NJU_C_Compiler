#ifndef NJU_C_COMPILER_LAB4_CODE_SYMBOL_TABLE_H_
#define NJU_C_COMPILER_LAB4_CODE_SYMBOL_TABLE_H_

#include <stdbool.h>

enum { kSymbolTableBucketCount = 16384 };

typedef struct Type Type;
typedef struct FieldList FieldList;
typedef struct Function Function;
typedef struct FunctionArg FunctionArg;

struct Type {
    enum { kBasicType, kArrayType, kStructType } kind;
    union {
        int basic;  // for kBasicType; 0 for int, 1 for float
        struct {
            Type* elem;
            int size;
            int elem_size;
        };  // for kArrayType
        struct {
            FieldList* structure;
            FieldList* tail;
            int stru_size;
        };  // for kStructType
    };
};

struct FieldList {
    Type* type;
    char* name;
    FieldList* next;
    int size, offset;
};

struct FunctionArg {
    Type* type;
    FunctionArg* next;
};

struct Function {
    char* name;
    Type* return_type;
    int argc;
    FunctionArg* argv;
    FunctionArg* tail;
};

/* 查找函数不修改名字；哈希值只是桶索引，不能作为符号唯一身份。 */
unsigned int HashSymbolName(const char* name);

/* 类型和函数对象由本次编译持有；名字借用语法树中的字符串。 */
/* 基本类型是共享的只读单例；数组及结构体类型单独分配。 */
Type* GetIntType(void);
Type* GetFloatType(void);

Type* CreateArrayType(Type* element_type, int size, int elem_size);

Type* CreateStructType(void);
FieldList* CreateField(char* name, Type* type);
void AddStructField(Type* struct_type, FieldList* field);

Function* CreateFunction(char* name, Type* return_type);
FunctionArg* CreateFunctionArgument(Type* type);
void AddFunctionArgument(Function* func, FunctionArg* arg);

/* 插入前由语义层检查重复定义；各类别使用独立名字表。 */
void InsertStructSymbol(char* name, Type* ty);
void InsertVariableSymbol(char* name, Type* ty);
void InsertFunctionSymbol(Function* func);

/* 数组忽略长度，结构体忽略字段名，比较类型与排列顺序。 */
Type* FindVariableSymbol(const char* name);
Type* FindStructSymbol(const char* name);
Function* FindFunctionSymbol(const char* name);
bool AreTypesEquivalent(Type* x, Type* y);
bool AreFunctionArgumentsCompatible(Function* funcx, Function* funcy);

#endif  // NJU_C_COMPILER_LAB4_CODE_SYMBOL_TABLE_H_
