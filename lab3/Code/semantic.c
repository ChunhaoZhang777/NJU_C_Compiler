/* 语义检查先建立类型与符号，再检查表达式；NULL 类型表示前面已诊断的错误。 */
#include "semantic.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"
#include "syntax_tree.h"

static void SemanticExtDefList(SyntaxNode* node);
static void SemanticExtDef(SyntaxNode* node);
static Type* SemanticSpecifier(SyntaxNode* node);
static Type* SemanticStructSpecifier(SyntaxNode* node);
static void SemanticDefListStruct(SyntaxNode* node, Type* stru);
static void SemanticDefStruct(SyntaxNode* node, Type* stru);
static void SemanticDecListStruct(SyntaxNode* node, Type* stru, Type* ty);
static void SemanticDecStruct(SyntaxNode* node, Type* stru, Type* ty);
static void SemanticVarDecStruct(SyntaxNode* node, Type* stru, Type* ty);
static void SemanticExtDecList(SyntaxNode* node, Type* ty);
static void SemanticVarDec(SyntaxNode* node, Type* ty);
static Function* SemanticFunDec(SyntaxNode* node, Type* return_type);
static void SemanticVarList(SyntaxNode* node, Function* func);
static void SemanticParamDec(SyntaxNode* node, Function* func);
static Type* SemanticVarDecFunc(SyntaxNode* node, Type* ty);
static void SemanticCompSt(SyntaxNode* node, Function* func);
static void SemanticDefList(SyntaxNode* node);
static void SemanticDef(SyntaxNode* node);
static void SemanticDecList(SyntaxNode* node, Type* ty);
static void SemanticDec(SyntaxNode* node, Type* ty);
static void SemanticStmtList(SyntaxNode* node, Function* func);
static void SemanticStmt(SyntaxNode* node, Function* func);
static void SemanticArgs(SyntaxNode* node, Function* func);
static bool IsLvalue(SyntaxNode* node);

static int g_semantic_error;

void DefineBuiltinFunctions(void) {
    Type* ty_int = GetIntType();
    Function* func_read = CreateFunction("read", ty_int);
    Function* func_write = CreateFunction("write", ty_int);
    FunctionArg* int_arg = CreateFunctionArgument(ty_int);
    AddFunctionArgument(func_write, int_arg);
    InsertFunctionSymbol(func_read);
    InsertFunctionSymbol(func_write);
}

int GetTypeSize(Type* ty) {
    if (ty == NULL)
        return 0;
    if (ty->kind == kBasicType) {
        return 4;
    } else if (ty->kind == kArrayType) {
        return ty->size * ty->elem_size;
    } else if (ty->kind == kStructType) {
        return ty->stru_size;
    }
    return 0;
}

static bool IsLvalue(SyntaxNode* node) {
    if (!strcmp(node->first_child->lex_name, "ID") && node->first_child->next == NULL) {
        return true;
    }
    if (FindSyntaxChild(node, "LB") != NULL) {
        return IsLvalue(node->first_child);
    }
    if (FindSyntaxChild(node, "DOT") != NULL) {
        return IsLvalue(node->first_child);
    }
    return false;
}

static void SemanticExtDefList(SyntaxNode* node) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "ExtDefList"));

    if (node->first_child == NULL) {
        // ExtDefList - epsilon
        return;
    }
    // ExtDefList - ExtDef ExtDefList
    SyntaxNode* node_ext_def = FindSyntaxChild(node, "ExtDef");
    SyntaxNode* node_ext_def_list = FindSyntaxChild(node, "ExtDefList");
    SemanticExtDef(node_ext_def);
    if (node_ext_def_list != NULL) {
        SemanticExtDefList(node_ext_def_list);
    }
}

static void SemanticExtDef(SyntaxNode* node) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "ExtDef"));

    Type* ty = SemanticSpecifier(node->first_child);

    SyntaxNode* node_ext_dec_list = FindSyntaxChild(node, "ExtDecList");
    SyntaxNode* node_fun_dec = FindSyntaxChild(node, "FunDec");

    if (node_ext_dec_list == NULL && node_fun_dec == NULL) {
        // ExtDef - Specifier SEMI
        return;
    } else if (node_ext_dec_list != NULL) {
        // ExtDef - Specifier ExtDecList SEMI
        SemanticExtDecList(node_ext_dec_list, ty);
    } else {
        // ExtDef - Specifier FunDec CompSt
        Function* func = SemanticFunDec(node_fun_dec, ty);
        Function* func_test = FindFunctionSymbol(func->name);
        if (func_test != NULL) {
            printf("Error type 4 at Line %d: Redefined function \'%s\'\n", node->line, func->name);
            g_semantic_error = 4;
        } else {
            InsertFunctionSymbol(func);
        }
        SemanticCompSt(FindSyntaxChild(node, "CompSt"), func);
    }
}

static Type* SemanticSpecifier(SyntaxNode* node) {
    assert(!strcmp(node->lex_name, "Specifier"));

    SyntaxNode* node_type = FindSyntaxChild(node, "TYPE");
    SyntaxNode* node_struct_specifier = FindSyntaxChild(node, "StructSpecifier");

    if (node_type != NULL) {
        // Specifier - TYPE
        if (!strcmp(node_type->content, "int")) {
            // TYPE == int
            return GetIntType();
        } else {
            // TYPE == float
            return GetFloatType();
        }
    }
    // Specifier - StructSpecifier
    return SemanticStructSpecifier(node_struct_specifier);
}

static Type* SemanticStructSpecifier(SyntaxNode* node) {
    assert(!strcmp(node->lex_name, "StructSpecifier"));

    SyntaxNode* node_tag = FindSyntaxChild(node, "Tag");

    if (node_tag != NULL) {
        // StructSpecifier - STRUCT Tag
        char* name = node_tag->first_child->content;
        Type* stru = FindStructSymbol(name);
        if (stru == NULL) {
            printf("Error type 17 at Line %d: Using undefined struct \'%s\'.\n", node->line, name);
            g_semantic_error = 17;
        }
        return stru;
    }
    // StructSpecifier - STRUCT OptTag LC DefList RC
    Type* stru = CreateStructType();

    SyntaxNode* node_def_list = FindSyntaxChild(node, "DefList");
    if (node_def_list != NULL) {
        SemanticDefListStruct(node_def_list, stru);
    }

    SyntaxNode* node_opt_tag = FindSyntaxChild(node, "OptTag");
    if (node_opt_tag != NULL) {
        char* name = node_opt_tag->first_child->content;
        Type* stru_test = FindStructSymbol(name);
        Type* symb_test = FindVariableSymbol(name);
        if (stru_test != NULL || symb_test != NULL) {
            printf("Error type 16 at Line %d: Redefined struct name \'%s\'.\n", node->line, name);
            g_semantic_error = 16;
        } else {
            InsertStructSymbol(name, stru);
        }
    }

    FieldList* cur = stru->structure;
    stru->stru_size = 0;
    while (cur != NULL) {
        cur->size = GetTypeSize(cur->type);
        cur->offset = stru->stru_size;
        stru->stru_size += cur->size;
        cur = cur->next;
    }

    return stru;
}

static void SemanticDefListStruct(SyntaxNode* node, Type* stru) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "DefList"));

    SyntaxNode* node_def = FindSyntaxChild(node, "Def");
    SyntaxNode* node_def_list = FindSyntaxChild(node, "DefList");

    if (node_def != NULL) {
        SemanticDefStruct(node_def, stru);
    }
    if (node_def_list != NULL) {
        SemanticDefListStruct(node_def_list, stru);
    }
}

static void SemanticDefStruct(SyntaxNode* node, Type* stru) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Def"));

    SyntaxNode* node_specifier = FindSyntaxChild(node, "Specifier");
    SyntaxNode* node_dec_list = FindSyntaxChild(node, "DecList");

    assert(node_specifier != NULL);
    assert(node_dec_list != NULL);

    Type* ty = SemanticSpecifier(node->first_child);
    SemanticDecListStruct(node->first_child->next, stru, ty);
}

static void SemanticDecListStruct(SyntaxNode* node, Type* stru, Type* ty) {  // ty might be NULL
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "DecList"));

    SyntaxNode* node_dec = FindSyntaxChild(node, "Dec");
    SyntaxNode* node_dec_list = FindSyntaxChild(node, "DecList");

    if (node_dec != NULL) {
        SemanticDecStruct(node_dec, stru, ty);
    }
    if (node_dec_list != NULL) {
        SemanticDecListStruct(node_dec_list, stru, ty);
    }
}

static void SemanticDecStruct(SyntaxNode* node, Type* stru, Type* ty) {  // ty might be NULL
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Dec"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_assignop = FindSyntaxChild(node, "ASSIGNOP");

    assert(node_var_dec != NULL);

    if (node_assignop != NULL) {
        printf("Error type 15 at Line %d: Assignment in struct definition.\n", node->line);
        g_semantic_error = 15;
    } else {
        SemanticVarDecStruct(node->first_child, stru, ty);
    }
}

static void SemanticVarDecStruct(SyntaxNode* node, Type* stru, Type* ty) {  // ty might be NULL
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "VarDec"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_id = FindSyntaxChild(node, "ID");

    if (node_var_dec != NULL) {
        // VarDec - VarDec LB INT RB
        SyntaxNode* node_int = FindSyntaxChild(node, "INT");
        SemanticVarDecStruct(node_var_dec, stru,
                             CreateArrayType(ty, node_int->int_value, GetTypeSize(ty)));
        return;
    }
    // VarDec - ID
    assert(node_id != NULL);
    FieldList* cur = stru->structure;
    while (cur != NULL) {
        if (!strcmp(cur->name, node_id->content)) {
            printf("Error type 15 at Line %d: Redefined field name \'%s\' in struct.\n", node->line,
                   cur->name);
            g_semantic_error = 15;
            return;
        }
        cur = cur->next;
    }
    AddStructField(stru, CreateField(node_id->content, ty));
}

static void SemanticExtDecList(SyntaxNode* node, Type* ty) {  // ty might be NULL
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "ExtDecList"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_ext_dec_list = FindSyntaxChild(node, "ExtDecList");

    if (node_var_dec != NULL) {
        SemanticVarDec(node_var_dec, ty);
    }
    if (node_ext_dec_list != NULL) {
        // ExtDecList - VarDec COMMA ExtDecList
        SemanticExtDecList(node_ext_dec_list, ty);
    }
}

static void SemanticVarDec(SyntaxNode* node, Type* ty) {  // ty might be NULL
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "VarDec"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_id = FindSyntaxChild(node, "ID");

    if (node_var_dec != NULL) {
        // VarDec - VarDec LB INT RB
        SyntaxNode* node_int = FindSyntaxChild(node, "INT");
        SemanticVarDec(node_var_dec, CreateArrayType(ty, node_int->int_value, GetTypeSize(ty)));
        return;
    }
    // VarDec - ID
    assert(node_id != NULL);
    char* name = node_id->content;
    Type* stru_test = FindStructSymbol(name);
    Type* symb_test = FindVariableSymbol(name);
    if (stru_test != NULL || symb_test != NULL) {
        printf("Error type 3 at Line %d: Redefined variable name \'%s\'.\n", node->line, name);
        g_semantic_error = 3;
    } else {
        InsertVariableSymbol(name, ty);
    }
}

static Function* SemanticFunDec(SyntaxNode* node, Type* return_type) {  // return_type might be NULL
    assert(!strcmp(node->lex_name, "FunDec"));
    Function* func = CreateFunction(node->first_child->content, return_type);

    SyntaxNode* node_var_list = FindSyntaxChild(node, "VarList");

    if (node_var_list == NULL) {
        // FunDec - ID LP RP
        return func;
    }
    // FunDec - ID LP VarList RP
    SemanticVarList(node_var_list, func);
    return func;
}

static void SemanticVarList(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "VarList"));

    SyntaxNode* node_param_dec = FindSyntaxChild(node, "ParamDec");
    SyntaxNode* node_var_list = FindSyntaxChild(node, "VarList");

    if (node_param_dec != NULL) {
        SemanticParamDec(node_param_dec, func);
    }
    if (node_var_list != NULL) {
        SemanticVarList(node_var_list, func);
    }
}

static void SemanticParamDec(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "ParamDec"));

    SyntaxNode* node_specifier = FindSyntaxChild(node, "Specifier");
    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");

    assert(node_specifier != NULL);
    assert(node_var_dec != NULL);

    Type* ty = SemanticVarDecFunc(node_var_dec, SemanticSpecifier(node_specifier));
    AddFunctionArgument(func, CreateFunctionArgument(ty));
}

static Type* SemanticVarDecFunc(SyntaxNode* node, Type* ty) {  // ty might be NULL
    assert(!strcmp(node->lex_name, "VarDec"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_id = FindSyntaxChild(node, "ID");

    if (node_var_dec != NULL) {
        // VarDec - VarDec LB INT RB
        SyntaxNode* node_int = FindSyntaxChild(node, "INT");
        return SemanticVarDecFunc(node_var_dec,
                                  CreateArrayType(ty, node_int->int_value, GetTypeSize(ty)));
    }
    // VarDec - ID
    assert(node_id != NULL);
    char* name = node_id->content;
    Type* stru_test = FindStructSymbol(name);
    Type* symb_test = FindVariableSymbol(name);
    if (stru_test != NULL || symb_test != NULL) {
        printf("Error type 3 at Line %d: Redefined variable name \'%s\'.\n", node->line, name);
        g_semantic_error = 3;
    } else {
        InsertVariableSymbol(name, ty);
    }
    return ty;
}

static void SemanticCompSt(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "CompSt"));

    SyntaxNode* node_def_list = FindSyntaxChild(node, "DefList");
    SyntaxNode* node_stmt_list = FindSyntaxChild(node, "StmtList");

    if (node_def_list != NULL) {
        SemanticDefList(node_def_list);
    }
    if (node_stmt_list != NULL) {
        SemanticStmtList(node_stmt_list, func);
    }
}

static void SemanticDefList(SyntaxNode* node) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "DefList"));

    SyntaxNode* node_def = FindSyntaxChild(node, "Def");
    SyntaxNode* node_def_list = FindSyntaxChild(node, "DefList");

    if (node_def != NULL) {
        SemanticDef(node_def);
    }
    if (node_def_list != NULL) {
        SemanticDefList(node_def_list);
    }
}

static void SemanticDef(SyntaxNode* node) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Def"));

    SyntaxNode* node_specifier = FindSyntaxChild(node, "Specifier");
    SyntaxNode* node_dec_list = FindSyntaxChild(node, "DecList");

    assert(node_specifier != NULL);
    assert(node_dec_list != NULL);

    Type* ty = SemanticSpecifier(node_specifier);
    SemanticDecList(node_dec_list, ty);
}

static void SemanticDecList(SyntaxNode* node, Type* ty) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "DecList"));

    SyntaxNode* node_dec = FindSyntaxChild(node, "Dec");
    SyntaxNode* node_dec_list = FindSyntaxChild(node, "DecList");

    if (node_dec != NULL) {
        SemanticDec(node_dec, ty);
    }
    if (node_dec_list != NULL) {
        SemanticDecList(node_dec_list, ty);
    }
}

static void SemanticDec(SyntaxNode* node, Type* ty) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Dec"));

    SyntaxNode* node_var_dec = FindSyntaxChild(node, "VarDec");
    SyntaxNode* node_assignop = FindSyntaxChild(node, "ASSIGNOP");

    assert(node_var_dec != NULL);

    SemanticVarDec(node_var_dec, ty);
    if (node_assignop != NULL) {
        // Dec - VarDec ASSIGNOP Exp
        SyntaxNode* cur = node;
        while (strcmp(cur->lex_name, "ID"))
            cur = cur->first_child;
        Type* ty_dec = FindVariableSymbol(cur->content);

        SyntaxNode* node_exp = FindSyntaxChild(node, "Exp");
        Type* ty_exp = AnalyzeExpressionType(node_exp);
        if (!AreTypesEquivalent(ty_dec, ty_exp)) {
            printf("Error type 5 at Line %d: Unmatching assignment.\n", node->line);
            g_semantic_error = 5;
        }
    }
}

static void SemanticStmtList(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "StmtList"));

    SyntaxNode* node_stmt = FindSyntaxChild(node, "Stmt");
    SyntaxNode* node_stmt_list = FindSyntaxChild(node, "StmtList");

    if (node_stmt != NULL) {
        SemanticStmt(node_stmt, func);
    }
    if (node_stmt_list != NULL) {
        SemanticStmtList(node_stmt_list, func);
    }
}

static void SemanticStmt(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Stmt"));

    SyntaxNode* node_comp_st = FindSyntaxChild(node, "CompSt");
    SyntaxNode* node_return = FindSyntaxChild(node, "RETURN");
    SyntaxNode* node_if = FindSyntaxChild(node, "IF");
    SyntaxNode* node_while = FindSyntaxChild(node, "WHILE");

    if (node_comp_st != NULL) {
        SemanticCompSt(node_comp_st, func);
        return;
    }
    if (node_return != NULL) {
        Type* return_type = AnalyzeExpressionType(FindSyntaxChild(node, "Exp"));
        if (!AreTypesEquivalent(return_type, func->return_type)) {
            printf("Error type 8 at Line %d: Unmatching return type.\n", node->line);
            g_semantic_error = 8;
        }
        return;
    }
    if (node_while != NULL) {
        Type* while_type = AnalyzeExpressionType(FindSyntaxChild(node, "Exp"));
        if (while_type && !AreTypesEquivalent(while_type, GetIntType())) {
            printf("Error type 7 at Line %d: While condition must have int type.\n", node->line);
            g_semantic_error = 7;
        }
        SemanticStmt(FindSyntaxChild(node, "Stmt"), func);
        return;
    }
    if (node_if != NULL) {
        Type* if_type = AnalyzeExpressionType(FindSyntaxChild(node, "Exp"));
        if (if_type && !AreTypesEquivalent(if_type, GetIntType())) {
            printf("Error type 7 at Line %d: If condition must have int type.\n", node->line);
            g_semantic_error = 7;
        }
        SemanticStmt(node->first_child->next->next->next->next, func);
        if (node->first_child->next->next->next->next->next != NULL) {
            SemanticStmt(node->first_child->next->next->next->next->next->next, func);
        }
        return;
    }
    AnalyzeExpressionType(FindSyntaxChild(node, "Exp"));
}

Type* AnalyzeExpressionType(SyntaxNode* node) {
    assert(!strcmp(node->lex_name, "Exp"));

    Type* ty_int = GetIntType();
    Type* ty_float = GetFloatType();

    if (FindSyntaxChild(node, "ASSIGNOP") != NULL) {
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child);
        Type* ty_exp2 = AnalyzeExpressionType(node->first_child->next->next);
        if (!AreTypesEquivalent(ty_exp1, ty_exp2)) {
            printf("Error type 5 at Line %d: Unmaching assignment.\n", node->line);
            g_semantic_error = 5;
        }
        if (!IsLvalue(node->first_child)) {
            printf("Error type 6 at Line %d: Assigning to a right value.\n", node->line);
            g_semantic_error = 6;
        }
        return ty_exp1;
    }
    if (FindSyntaxChild(node, "AND") != NULL || FindSyntaxChild(node, "OR") != NULL) {
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child);
        Type* ty_exp2 = AnalyzeExpressionType(node->first_child->next->next);
        if (!AreTypesEquivalent(ty_exp1, ty_int) || !AreTypesEquivalent(ty_exp2, ty_int)) {
            printf("Error type 7 at Line %d: Logic operands only accept int values.\n", node->line);
            g_semantic_error = 7;
        }
        return ty_int;
    }
    if (FindSyntaxChild(node, "NOT") != NULL) {
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child->next);
        if (!AreTypesEquivalent(ty_exp1, ty_int)) {
            printf("Error type 7 at Line %d: Logic operands only accept int values.\n", node->line);
            g_semantic_error = 7;
        }
        return ty_int;
    }
    if (!strcmp(node->first_child->lex_name, "LP")) {
        return AnalyzeExpressionType(node->first_child->next);
    }
    if (FindSyntaxChild(node, "RELOP") != NULL || FindSyntaxChild(node, "PLUS") != NULL ||
        FindSyntaxChild(node, "STAR") != NULL || FindSyntaxChild(node, "DIV") != NULL) {
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child);
        Type* ty_exp2 = AnalyzeExpressionType(node->first_child->next->next);

        if ((!AreTypesEquivalent(ty_exp1, ty_int) || !AreTypesEquivalent(ty_exp2, ty_int)) &&
            (!AreTypesEquivalent(ty_exp1, ty_float) || !AreTypesEquivalent(ty_exp2, ty_float))) {
            printf("Error type 7 at Line %d: Unaccepted operand type.\n", node->line);
            g_semantic_error = 7;
        }
        if (!strcmp(node->first_child->next->lex_name, "RELOP")) {
            return ty_int;
        } else {
            return ty_exp1;
        }
    }
    if (FindSyntaxChild(node, "MINUS") != NULL) {
        if (!strcmp(node->first_child->lex_name, "MINUS")) {
            Type* ty_exp = AnalyzeExpressionType(node->first_child->next);
            if (!AreTypesEquivalent(ty_exp, ty_int) && !AreTypesEquivalent(ty_exp, ty_float)) {
                printf("Error type 7 at Line %d: Unaccepted operand type.\n", node->line);
                g_semantic_error = 7;
            }
            return ty_exp;
        }
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child);
        Type* ty_exp2 = AnalyzeExpressionType(node->first_child->next->next);

        if ((!AreTypesEquivalent(ty_exp1, ty_int) || !AreTypesEquivalent(ty_exp2, ty_int)) &&
            (!AreTypesEquivalent(ty_exp1, ty_float) || !AreTypesEquivalent(ty_exp2, ty_float))) {
            printf("Error type 7 at Line %d: Unaccepted operand type.\n", node->line);
            g_semantic_error = 7;
        }
        return ty_exp1;
    }
    if (!strcmp(node->first_child->lex_name, "INT")) {
        return ty_int;
    }
    if (!strcmp(node->first_child->lex_name, "FLOAT")) {
        return ty_float;
    }
    if (FindSyntaxChild(node, "LB") != NULL) {
        Type* ty_exp1 = AnalyzeExpressionType(node->first_child);
        Type* ty_exp2 = AnalyzeExpressionType(node->first_child->next->next);
        if (!AreTypesEquivalent(ty_exp2, ty_int)) {
            printf("Error type 12 at Line %d: Cannot use non-int as array index.\n", node->line);
            g_semantic_error = 12;
        }
        if (ty_exp1 == NULL)
            return NULL;
        if (ty_exp1->kind != kArrayType) {
            printf("Error type 10 at Line %d: Cannot index a non-array.\n", node->line);
            g_semantic_error = 10;
            return ty_exp1;
        }
        return ty_exp1->elem;
    }
    if (FindSyntaxChild(node, "DOT") != NULL) {
        Type* ty_exp = AnalyzeExpressionType(node->first_child);
        if (ty_exp == NULL)
            return NULL;
        if (ty_exp->kind != kStructType) {
            printf("Error type 13 at Line %d: Cannot field a non-struct.\n", node->line);
            g_semantic_error = 13;
            return ty_exp;
        }
        char* field_name = node->first_child->next->next->content;
        FieldList* cur = ty_exp->structure;
        while (cur != NULL) {
            if (strcmp(cur->name, field_name)) {
                cur = cur->next;
                continue;
            }
            return cur->type;
        }
        printf("Error type 14 at Line %d: Undefined field \'%s\'.\n", node->line, field_name);
        g_semantic_error = 14;
        return ty_exp;
    }
    assert(!strcmp(node->first_child->lex_name, "ID"));
    if (node->first_child->next == NULL) {
        Type* ty = FindVariableSymbol(node->first_child->content);
        if (ty == NULL) {
            printf("Error type 1 at Line %d: Undefined variable \'%s\'.\n", node->line,
                   node->first_child->content);
            g_semantic_error = 1;
            return NULL;
        }
        return ty;
    }
    Function* fake_func = CreateFunction("<CallerArgs>", NULL);
    if (FindSyntaxChild(node, "Args")) {
        SemanticArgs(node->first_child->next->next, fake_func);
    }
    char* func_name = node->first_child->content;
    Function* func = FindFunctionSymbol(func_name);
    if (FindVariableSymbol(func_name) != NULL) {
        printf("Error type 11 at Line %d: Cannot call a variable \'%s\'.\n", node->line, func_name);
        g_semantic_error = 11;
        return NULL;
    }
    if (func == NULL) {
        printf("Error type 2 at Line %d: Undefined function \'%s\'.\n", node->line, func_name);
        g_semantic_error = 2;
        return NULL;
    }
    if (!AreFunctionArgumentsCompatible(func, fake_func)) {
        printf("Error type 9 at Line %d: Unmaching args for function \'%s\'.\n", node->line,
               func_name);
        g_semantic_error = 9;
        return NULL;
    }
    return func->return_type;
}

static void SemanticArgs(SyntaxNode* node, Function* func) {
    if (node == NULL)
        return;
    assert(!strcmp(node->lex_name, "Args"));

    SyntaxNode* node_args = FindSyntaxChild(node, "Args");
    AddFunctionArgument(func, CreateFunctionArgument(AnalyzeExpressionType(node->first_child)));
    if (node_args != NULL) {
        SemanticArgs(node->first_child->next->next, func);
    }
}

int AnalyzeProgram(SyntaxNode* program) {
    SemanticExtDefList(program ? program->first_child : NULL);
    return g_semantic_error != 0;
}
