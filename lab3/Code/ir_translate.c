/* 翻译已通过语义检查的整数程序；短路条件与聚合类型地址在此阶段展开。 */
#include "ir_translate.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "semantic.h"
#include "symbol_table.h"
#include "syntax_tree.h"

typedef struct OffsetQuery OffsetQuery;
struct OffsetQuery {
    IrList* ic;
    Type* ty;
};

typedef struct ArgList ArgList;
typedef struct ArgId ArgId;
struct ArgId {
    char* operand;
    struct ArgId* next;
};
struct ArgList {
    ArgId* head;
};

static IrList* IrTranslateExtDefList(SyntaxNode* node);
static IrList* IrTranslateExtDef(SyntaxNode* node);
static IrList* IrTranslateFunDec(SyntaxNode* node);
static IrList* IrTranslateVarList(SyntaxNode* node);
static IrList* IrTranslateParamDec(SyntaxNode* node);
static IrList* IrTranslateVarDecFunction(SyntaxNode* node);
static IrList* IrTranslateCompSt(SyntaxNode* node);
static IrList* IrTranslateDefList(SyntaxNode* node);
static IrList* IrTranslateDef(SyntaxNode* node);
static IrList* IrTranslateDecList(SyntaxNode* node);
static IrList* IrTranslateDec(SyntaxNode* node);
static IrList* IrTranslateStmtList(SyntaxNode* node);
static IrList* IrTranslateStmt(SyntaxNode* node);
static IrList* IrTranslateExp(SyntaxNode* node, char* place);
static IrList* IrTranslateCond(SyntaxNode* node, char* label_true, char* label_false);
static OffsetQuery IrTranslateExpOffset(SyntaxNode* node, char* place);
static IrList* IrTranslateArgs(SyntaxNode* node, ArgList* arg_list);

/* 仅折叠无副作用且结果可表示为 int32 的常量表达式；溢出和除零留到运行时。 */
static bool EvaluateConstantExpression(SyntaxNode* node, int* result) {
    SyntaxNode* first = node->first_child;
    if (!strcmp(first->lex_name, "INT")) {
        if (first->int_value > INT_MAX)
            return false;
        *result = (int)first->int_value;
        return true;
    }
    if (!strcmp(first->lex_name, "LP"))
        return EvaluateConstantExpression(first->next, result);
    int left, right;
    if (!strcmp(first->lex_name, "MINUS")) {
        if (!EvaluateConstantExpression(first->next, &right) || right == INT_MIN)
            return false;
        *result = -right;
        return true;
    }
    if (strcmp(first->lex_name, "Exp") || !first->next || !first->next->next)
        return false;
    char* op = first->next->lex_name;
    if (strcmp(op, "PLUS") && strcmp(op, "MINUS") && strcmp(op, "STAR") && strcmp(op, "DIV"))
        return false;
    if (!EvaluateConstantExpression(first, &left) ||
        !EvaluateConstantExpression(first->next->next, &right))
        return false;
    int64_t value;
    if (!strcmp(op, "PLUS"))
        value = (int64_t)left + right;
    else if (!strcmp(op, "MINUS"))
        value = (int64_t)left - right;
    else if (!strcmp(op, "STAR"))
        value = (int64_t)left * right;
    else {
        if (right == 0 || (left == INT_MIN && right == -1))
            return false;
        value = left / right;
    }
    if (value < INT_MIN || value > INT_MAX)
        return false;
    *result = (int)value;
    return true;
}

/* 按两侧各自布局复制对应字段/元素；保持基址不变，保证连续赋值的结果地址有效。 */
static void EmitAggregateCopy(IrList* codes, char* dest, char* src, Type* dest_type, Type* src_type,
                              int dest_offset, int src_offset) {
    if (dest_type->kind == kArrayType) {
        int count = dest_type->size < src_type->size ? dest_type->size : src_type->size;
        for (int i = 0; i < count; ++i)
            EmitAggregateCopy(codes, dest, src, dest_type->elem, src_type->elem,
                              dest_offset + i * dest_type->elem_size,
                              src_offset + i * src_type->elem_size);
    } else if (dest_type->kind == kStructType) {
        FieldList* left = dest_type->structure;
        FieldList* right = src_type->structure;
        for (; left && right; left = left->next, right = right->next)
            EmitAggregateCopy(codes, dest, src, left->type, right->type, dest_offset + left->offset,
                              src_offset + right->offset);
    } else {
        char* left = dest;
        char* right = src;
        if (dest_offset) {
            left = CreateIrTemporary();
            AppendIrInstruction(
                codes, CreateIrInstruction(kIrAdd, left, dest, CreateIrConstant(dest_offset)));
        }
        if (src_offset) {
            right = CreateIrTemporary();
            AppendIrInstruction(
                codes, CreateIrInstruction(kIrAdd, right, src, CreateIrConstant(src_offset)));
        }
        char* value = CreateIrTemporary();
        AppendIrInstruction(codes, CreateIrInstruction(kIrLoad, value, right));
        AppendIrInstruction(codes, CreateIrInstruction(kIrStore, left, value));
    }
}

static IrList* IrTranslateExtDefList(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "ExtDefList"));

    IrList* ext_def_codes = IrTranslateExtDef(FindSyntaxChild(node, "ExtDef"));
    IrList* ext_def_list_codes = IrTranslateExtDefList(FindSyntaxChild(node, "ExtDefList"));

    AppendIrList(ext_def_codes, ext_def_list_codes);

    return ext_def_codes;
}

static IrList* IrTranslateExtDef(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "ExtDef"));

    SyntaxNode* fun_dec_node = FindSyntaxChild(node, "FunDec");
    if (fun_dec_node != NULL) {
        IrList* label_codes =
            WrapIrInstruction(CreateIrInstruction(kIrFunction, fun_dec_node->first_child->content));
        IrList* fun_dec_codes = IrTranslateFunDec(fun_dec_node);
        IrList* comp_st_codes = IrTranslateCompSt(FindSyntaxChild(node, "CompSt"));
        AppendIrList(fun_dec_codes, comp_st_codes);
        AppendIrList(label_codes, fun_dec_codes);
        return label_codes;
    } else {
        return CreateIrList();
    }
}

static IrList* IrTranslateFunDec(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "FunDec"));

    SyntaxNode* var_list_node = FindSyntaxChild(node, "VarList");
    return IrTranslateVarList(var_list_node);
}

static IrList* IrTranslateVarList(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "VarList"));

    IrList* param_dec_codes = IrTranslateParamDec(FindSyntaxChild(node, "ParamDec"));
    IrList* var_list_codes = IrTranslateVarList(FindSyntaxChild(node, "VarList"));
    AppendIrList(param_dec_codes, var_list_codes);
    return param_dec_codes;
}

static IrList* IrTranslateParamDec(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "ParamDec"));

    return IrTranslateVarDecFunction(FindSyntaxChild(node, "VarDec"));
}

static IrList* IrTranslateVarDecFunction(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "VarDec"));

    if (!strcmp(node->first_child->lex_name, "ID")) {
        RegisterIrParameter(node->first_child->content);
        return WrapIrInstruction(CreateIrInstruction(kIrParameter, node->first_child->content));
    } else {
        return IrTranslateVarDecFunction(FindSyntaxChild(node, "VarDec"));
    }
}

static IrList* IrTranslateCompSt(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "CompSt"));

    IrList* def_list_codes = IrTranslateDefList(FindSyntaxChild(node, "DefList"));
    IrList* stmt_list_codes = IrTranslateStmtList(FindSyntaxChild(node, "StmtList"));
    AppendIrList(def_list_codes, stmt_list_codes);
    return def_list_codes;
}

static IrList* IrTranslateDefList(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "DefList"));

    IrList* def_codes = IrTranslateDef(FindSyntaxChild(node, "Def"));
    IrList* def_list_codes = IrTranslateDefList(FindSyntaxChild(node, "DefList"));
    AppendIrList(def_codes, def_list_codes);
    return def_codes;
}

static IrList* IrTranslateDef(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "Def"));

    return IrTranslateDecList(FindSyntaxChild(node, "DecList"));
}

static IrList* IrTranslateDecList(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "DecList"));

    IrList* dec_codes = IrTranslateDec(FindSyntaxChild(node, "Dec"));
    IrList* dec_list_codes = IrTranslateDecList(FindSyntaxChild(node, "DecList"));
    AppendIrList(dec_codes, dec_list_codes);
    return dec_codes;
}

static IrList* IrTranslateDec(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "Dec"));

    char* id;
    SyntaxNode* cur = node;
    while (strcmp(cur->lex_name, "ID"))
        cur = cur->first_child;
    id = cur->content;

    Type* ty = FindVariableSymbol(id);
    IrList* res = CreateIrList();
    if (ty->kind == kStructType) {
        AppendIrInstruction(res, CreateIrInstruction(kIrAllocate, id, ty->stru_size));
    } else if (ty->kind == kArrayType) {
        AppendIrInstruction(res, CreateIrInstruction(kIrAllocate, id, ty->size * ty->elem_size));
    }

    if (FindSyntaxChild(node, "ASSIGNOP") != NULL) {
        if (ty->kind == kBasicType) {
            char* t1 = CreateIrTemporary();
            IrList* exp_codes = IrTranslateExp(FindSyntaxChild(node, "Exp"), t1);
            AppendIrInstruction(exp_codes, CreateIrInstruction(kIrAssign, id, t1));
            return exp_codes;
        } else {
            char* t1 = CreateIrTemporary();
            IrList* exp_codes = IrTranslateExp(FindSyntaxChild(node, "Exp"), t1);
            AppendIrList(res, exp_codes);
            char* laddr = CreateIrTemporary();
            char* raddr = t1;
            AppendIrInstruction(res, CreateIrInstruction(kIrAddressOf, laddr, id));

            EmitAggregateCopy(res, laddr, raddr, ty,
                              AnalyzeExpressionType(FindSyntaxChild(node, "Exp")), 0, 0);

            return res;
        }
    }
    return res;
}

static IrList* IrTranslateStmtList(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "StmtList"));

    IrList* stmt_codes = IrTranslateStmt(FindSyntaxChild(node, "Stmt"));
    IrList* stmt_list_codes = IrTranslateStmtList(FindSyntaxChild(node, "StmtList"));
    AppendIrList(stmt_codes, stmt_list_codes);
    return stmt_codes;
}

static IrList* IrTranslateStmt(SyntaxNode* node) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "Stmt"));

    // Exp SEMI
    if (!strcmp(node->first_child->lex_name, "Exp")) {
        return IrTranslateExp(FindSyntaxChild(node, "Exp"), CreateIrTemporary());
    }
    // CompSt
    if (FindSyntaxChild(node, "CompSt")) {
        return IrTranslateCompSt(node->first_child);
    }
    // RETURN Exp SEMI
    if (FindSyntaxChild(node, "RETURN")) {
        char* t1 = CreateIrTemporary();
        IrList* codes = IrTranslateExp(FindSyntaxChild(node, "Exp"), t1);
        AppendIrInstruction(codes, CreateIrInstruction(kIrReturn, t1));
        return codes;
    }
    // WHILE LP Exp RP Stmt
    if (FindSyntaxChild(node, "WHILE")) {
        char* label1 = CreateIrLabel();
        char* label2 = CreateIrLabel();
        char* label3 = CreateIrLabel();
        IrList* code1 = IrTranslateCond(FindSyntaxChild(node, "Exp"), label2, label3);
        IrList* code2 = IrTranslateStmt(FindSyntaxChild(node, "Stmt"));

        IrList* ans = WrapIrInstruction(CreateIrInstruction(kIrLabel, label1));
        AppendIrList(ans, code1);
        AppendIrInstruction(ans, CreateIrInstruction(kIrLabel, label2));
        AppendIrList(ans, code2);
        AppendIrInstruction(ans, CreateIrInstruction(kIrGoto, label1));
        AppendIrInstruction(ans, CreateIrInstruction(kIrLabel, label3));
        return ans;
    }
    // IF LP Exp RP Stmt
    if (FindSyntaxChild(node, "IF") && !FindSyntaxChild(node, "ELSE")) {
        char* label1 = CreateIrLabel();
        char* label2 = CreateIrLabel();
        IrList* code1 = IrTranslateCond(FindSyntaxChild(node, "Exp"), label1, label2);
        IrList* code2 = IrTranslateStmt(FindSyntaxChild(node, "Stmt"));
        AppendIrInstruction(code1, CreateIrInstruction(kIrLabel, label1));
        AppendIrList(code1, code2);
        AppendIrInstruction(code1, CreateIrInstruction(kIrLabel, label2));
        return code1;
    }
    // IF LP Exp RP Stmt ELSE Stmt
    if (FindSyntaxChild(node, "ELSE")) {
        char* label1 = CreateIrLabel();
        char* label2 = CreateIrLabel();
        char* label3 = CreateIrLabel();
        IrList* code1 = IrTranslateCond(FindSyntaxChild(node, "Exp"), label1, label2);
        IrList* code2 = IrTranslateStmt(node->first_child->next->next->next->next);
        IrList* code3 = IrTranslateStmt(node->first_child->next->next->next->next->next->next);
        AppendIrInstruction(code1, CreateIrInstruction(kIrLabel, label1));
        AppendIrList(code1, code2);
        AppendIrInstruction(code1, CreateIrInstruction(kIrGoto, label3));
        AppendIrInstruction(code1, CreateIrInstruction(kIrLabel, label2));
        AppendIrList(code1, code3);
        AppendIrInstruction(code1, CreateIrInstruction(kIrLabel, label3));
        return code1;
    }
    return CreateIrList();
}

static IrList* IrTranslateExp(SyntaxNode* node, char* place) {
    if (node == NULL)
        return CreateIrList();
    assert(!strcmp(node->lex_name, "Exp"));
    int folded;
    if (EvaluateConstantExpression(node, &folded))
        return WrapIrInstruction(CreateIrInstruction(kIrAssign, place, CreateIrConstant(folded)));
    // INT
    if (!strcmp(node->first_child->lex_name, "INT")) {
        return WrapIrInstruction(CreateIrInstruction(
            kIrAssign, place, CreateIrConstant((int)node->first_child->int_value)));
    }
    // FLOAT
    if (!strcmp(node->first_child->lex_name, "FLOAT")) {
        char* float_str = malloc(strlen(node->first_child->content) + 2);
        float_str[0] = '#';
        strcpy(float_str + 1, node->first_child->content);
        return WrapIrInstruction(CreateIrInstruction(kIrAssign, place, float_str));
    }
    // ID
    if (!strcmp(node->first_child->lex_name, "ID") && node->first_child->next == NULL) {
        char* id = node->first_child->content;
        if (FindVariableSymbol(id)->kind != kBasicType) {
            if (IsIrParameter(id)) {
                return WrapIrInstruction(CreateIrInstruction(kIrAssign, place, id));
            } else {
                return WrapIrInstruction(CreateIrInstruction(kIrAddressOf, place, id));
            }
        } else {
            return WrapIrInstruction(CreateIrInstruction(kIrAssign, place, id));
        }
    }
    // ASSIGNOP
    if (FindSyntaxChild(node, "ASSIGNOP") != NULL) {
        char* tval = CreateIrTemporary();
        IrList* r_exp_codes = IrTranslateExp(node->first_child->next->next, tval);
        if (!strcmp(node->first_child->first_child->lex_name, "ID")) {
            char* id = node->first_child->first_child->content;
            Type* ty = FindVariableSymbol(id);
            if (ty->kind == kBasicType) {
                AppendIrInstruction(
                    r_exp_codes,
                    CreateIrInstruction(kIrAssign, node->first_child->first_child->content, tval));
                AppendIrInstruction(r_exp_codes, CreateIrInstruction(kIrAssign, place, tval));
                return r_exp_codes;
            } else {
                char* laddr = CreateIrTemporary();
                AppendIrInstruction(
                    r_exp_codes,
                    CreateIrInstruction(IsIrParameter(id) ? kIrAssign : kIrAddressOf, laddr, id));

                char* raddr = tval;

                AppendIrInstruction(r_exp_codes, CreateIrInstruction(kIrAssign, place, laddr));
                EmitAggregateCopy(r_exp_codes, laddr, raddr, ty,
                                  AnalyzeExpressionType(node->first_child->next->next), 0, 0);

                return r_exp_codes;
            }
        }
        char* toffset = CreateIrTemporary();
        OffsetQuery info = IrTranslateExpOffset(node->first_child, toffset);
        IrList* l_exp_codes = info.ic;

        char* id;
        SyntaxNode* cur = node;
        while (strcmp(cur->lex_name, "ID"))
            cur = cur->first_child;
        id = cur->content;

        if (info.ty->kind == kBasicType) {
            char* taddr = CreateIrTemporary();
            if (IsIrParameter(id)) {
                AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAssign, taddr, id));
            } else {
                AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAddressOf, taddr, id));
            }
            AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAdd, taddr, taddr, toffset));
            AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrStore, taddr, tval));
            AppendIrList(r_exp_codes, l_exp_codes);
            AppendIrInstruction(r_exp_codes, CreateIrInstruction(kIrAssign, place, tval));
            return r_exp_codes;
        } else {
            char* laddr = CreateIrTemporary();
            if (IsIrParameter(id)) {
                AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAssign, laddr, id));
            } else {
                AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAddressOf, laddr, id));
            }
            AppendIrInstruction(l_exp_codes, CreateIrInstruction(kIrAdd, laddr, laddr, toffset));
            AppendIrList(r_exp_codes, l_exp_codes);

            char* raddr = tval;

            AppendIrInstruction(r_exp_codes, CreateIrInstruction(kIrAssign, place, laddr));
            EmitAggregateCopy(r_exp_codes, laddr, raddr, info.ty,
                              AnalyzeExpressionType(node->first_child->next->next), 0, 0);

            return r_exp_codes;
        }
    }
    // PLUS
    if (FindSyntaxChild(node, "PLUS") != NULL) {
        char* t1 = CreateIrTemporary();
        char* t2 = CreateIrTemporary();
        IrList* codes1 = IrTranslateExp(node->first_child, t1);
        IrList* codes2 = IrTranslateExp(node->first_child->next->next, t2);
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrAdd, place, t1, t2));
        return codes1;
    }
    // MINUS
    if (FindSyntaxChild(node, "MINUS") != NULL && strcmp(node->first_child->lex_name, "MINUS")) {
        char* t1 = CreateIrTemporary();
        char* t2 = CreateIrTemporary();
        IrList* codes1 = IrTranslateExp(node->first_child, t1);
        IrList* codes2 = IrTranslateExp(node->first_child->next->next, t2);
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrSubtract, place, t1, t2));
        return codes1;
    }
    // PROD
    if (FindSyntaxChild(node, "STAR") != NULL) {
        char* t1 = CreateIrTemporary();
        char* t2 = CreateIrTemporary();
        IrList* codes1 = IrTranslateExp(node->first_child, t1);
        IrList* codes2 = IrTranslateExp(node->first_child->next->next, t2);
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrMultiply, place, t1, t2));
        return codes1;
    }
    // DIV
    if (FindSyntaxChild(node, "DIV") != NULL) {
        char* t1 = CreateIrTemporary();
        char* t2 = CreateIrTemporary();
        IrList* codes1 = IrTranslateExp(node->first_child, t1);
        IrList* codes2 = IrTranslateExp(node->first_child->next->next, t2);
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrDivide, place, t1, t2));
        return codes1;
    }
    // MINUS(NEG)
    if (!strcmp(node->first_child->lex_name, "MINUS")) {
        char* t1 = CreateIrTemporary();
        IrList* codes = IrTranslateExp(node->first_child->next, t1);
        AppendIrInstruction(codes, CreateIrInstruction(kIrSubtract, place, "#0", t1));
        return codes;
    }
    // LP Exp RP
    if (!strcmp(node->first_child->lex_name, "LP")) {
        return IrTranslateExp(FindSyntaxChild(node, "Exp"), place);
    }
    // Cond: AND OR NOT RELOP
    if (FindSyntaxChild(node, "AND") || FindSyntaxChild(node, "OR") ||
        FindSyntaxChild(node, "NOT") || FindSyntaxChild(node, "RELOP")) {
        char* label1 = CreateIrLabel();
        char* label2 = CreateIrLabel();
        IrList* ans = WrapIrInstruction(CreateIrInstruction(kIrAssign, place, "#0"));
        AppendIrList(ans, IrTranslateCond(node, label1, label2));
        AppendIrInstruction(ans, CreateIrInstruction(kIrLabel, label1));
        AppendIrInstruction(ans, CreateIrInstruction(kIrAssign, place, "#1"));
        AppendIrInstruction(ans, CreateIrInstruction(kIrLabel, label2));
        return ans;
    }
    // ID LP RP
    if (FindSyntaxChild(node, "LP") && !FindSyntaxChild(node, "Args")) {
        assert(!strcmp(node->first_child->lex_name, "ID"));
        char* id = node->first_child->content;
        if (!strcmp(id, "read")) {
            return WrapIrInstruction(CreateIrInstruction(kIrRead, place));
        }
        return WrapIrInstruction(CreateIrInstruction(kIrCall, place, id));
    }
    // ID LP Args RP
    if (FindSyntaxChild(node, "LP") && FindSyntaxChild(node, "Args")) {
        assert(!strcmp(node->first_child->lex_name, "ID"));
        char* id = node->first_child->content;

        ArgList* arg_list = malloc(sizeof(ArgList));
        arg_list->head = NULL;
        IrList* codes1 = IrTranslateArgs(FindSyntaxChild(node, "Args"), arg_list);
        if (!strcmp(id, "write")) {
            AppendIrInstruction(codes1, CreateIrInstruction(kIrWrite, arg_list->head->operand));
            AppendIrInstruction(codes1, CreateIrInstruction(kIrAssign, place, "#0"));
            return codes1;
        }

        IrList* codes2 = CreateIrList();
        ArgId* cur_arg = arg_list->head;
        Function* func = FindFunctionSymbol(id);
        FunctionArg* declared_arg = func->argv;

        while (cur_arg != NULL) {
            IrList* cur_codes =
                WrapIrInstruction(CreateIrInstruction(kIrArgument, cur_arg->operand));
            AppendIrList(codes2, cur_codes);
            cur_arg = cur_arg->next;
            declared_arg = declared_arg->next;
        }
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrCall, place, id));
        return codes1;
    }
    // Exp DOT ID
    // Exp LB Exp RB
    if (FindSyntaxChild(node, "DOT") || FindSyntaxChild(node, "LB")) {
        char* source_id = NULL;
        SyntaxNode* cur = node;
        while (strcmp(cur->lex_name, "ID"))
            cur = cur->first_child;
        source_id = cur->content;

        char* taddr = place;
        IrList* ans;
        if (IsIrParameter(source_id)) {
            ans = WrapIrInstruction(CreateIrInstruction(kIrAssign, taddr, source_id));
        } else {
            ans = WrapIrInstruction(CreateIrInstruction(kIrAddressOf, taddr, source_id));
        }

        char* toffset = CreateIrTemporary();
        OffsetQuery info = IrTranslateExpOffset(node, toffset);
        AppendIrList(ans, info.ic);
        AppendIrInstruction(ans, CreateIrInstruction(kIrAdd, taddr, taddr, toffset));

        if (info.ty->kind == kBasicType) {
            AppendIrInstruction(ans, CreateIrInstruction(kIrLoad, taddr, taddr));
        }
        return ans;
    }

    printf("Fatal Error.\n");
    return CreateIrList();
}

static OffsetQuery IrTranslateExpOffset(SyntaxNode* node, char* place) {
    assert(!strcmp(node->lex_name, "Exp"));
    // node is guaranteed to be one of:
    //     Exp -> ID
    //     Exp -> Exp LB Exp RB
    //     Exp -> Exp DOT ID
    // Example: j[c+10].ss[g*5].sb = 9;
    if (FindSyntaxChild(node, "LB") != NULL) {
        OffsetQuery info = IrTranslateExpOffset(node->first_child, place);
        assert(info.ty->kind == kArrayType);

        char* tidx = CreateIrTemporary();
        IrList* cur_offset = IrTranslateExp(node->first_child->next->next, tidx);
        char* elem_size_str = CreateIrConstant(info.ty->elem_size);
        AppendIrInstruction(cur_offset,
                            CreateIrInstruction(kIrMultiply, tidx, tidx, elem_size_str));
        AppendIrInstruction(cur_offset, CreateIrInstruction(kIrAdd, place, place, tidx));

        AppendIrList(info.ic, cur_offset);

        OffsetQuery ans;
        ans.ic = info.ic;
        ans.ty = info.ty->elem;
        return ans;
    } else if (FindSyntaxChild(node, "DOT") != NULL) {
        OffsetQuery info = IrTranslateExpOffset(node->first_child, place);
        assert(info.ty->kind == kStructType);

        FieldList* cur = info.ty->structure;
        while (strcmp(cur->name, node->first_child->next->next->content))
            cur = cur->next;
        char* field_offset_str = CreateIrConstant(cur->offset);

        AppendIrInstruction(info.ic, CreateIrInstruction(kIrAdd, place, place, field_offset_str));
        info.ty = cur->type;
        return info;
    } else {
        SyntaxNode* id_node = node->first_child;
        assert(!strcmp(id_node->lex_name, "ID"));
        Type* ty = FindVariableSymbol(id_node->content);

        OffsetQuery ans;
        ans.ic = WrapIrInstruction(CreateIrInstruction(kIrAssign, place, "#0"));
        ans.ty = ty;
        return ans;
    }
}

/* 与、或通过标签控制求值顺序，右侧有副作用时也保持短路语义。 */
static IrList* IrTranslateCond(SyntaxNode* node, char* label_true, char* label_false) {
    assert(!strcmp(node->lex_name, "Exp"));

    if (FindSyntaxChild(node, "RELOP") != NULL) {
        char* t1 = CreateIrTemporary();
        char* t2 = CreateIrTemporary();
        IrList* codes1 = IrTranslateExp(node->first_child, t1);
        IrList* codes2 = IrTranslateExp(node->first_child->next->next, t2);
        char* relop_str = node->first_child->next->content;
        assert(!strcmp(node->first_child->next->lex_name, "RELOP"));
        IrRelation* op = malloc(sizeof(IrRelation));
        if (!strcmp(relop_str, "<"))
            op->kind = kRelLess;
        if (!strcmp(relop_str, ">"))
            op->kind = kRelGreater;
        if (!strcmp(relop_str, "<="))
            op->kind = kRelLessEqual;
        if (!strcmp(relop_str, ">="))
            op->kind = kRelGreaterEqual;
        if (!strcmp(relop_str, "=="))
            op->kind = kRelEqual;
        if (!strcmp(relop_str, "!="))
            op->kind = kRelNotEqual;
        AppendIrList(codes1, codes2);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrBranchIf, t1, op, t2, label_true));
        AppendIrInstruction(codes1, CreateIrInstruction(kIrGoto, label_false));
        return codes1;
    } else if (FindSyntaxChild(node, "NOT")) {
        return IrTranslateCond(node->first_child->next, label_false, label_true);
    } else if (FindSyntaxChild(node, "AND")) {
        char* label1 = CreateIrLabel();
        IrList* codes1 = IrTranslateCond(node->first_child, label1, label_false);
        IrList* codes2 = IrTranslateCond(node->first_child->next->next, label_true, label_false);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrLabel, label1));
        AppendIrList(codes1, codes2);
        return codes1;
    } else if (FindSyntaxChild(node, "OR")) {
        char* label1 = CreateIrLabel();
        IrList* codes1 = IrTranslateCond(node->first_child, label_true, label1);
        IrList* codes2 = IrTranslateCond(node->first_child->next->next, label_true, label_false);
        AppendIrInstruction(codes1, CreateIrInstruction(kIrLabel, label1));
        AppendIrList(codes1, codes2);
        return codes1;
    } else {
        char* t1 = CreateIrTemporary();
        IrList* codes = IrTranslateExp(node, t1);
        IrRelation* op = malloc(sizeof(IrRelation));
        op->kind = kRelNotEqual;
        AppendIrInstruction(codes, CreateIrInstruction(kIrBranchIf, t1, op, "#0", label_true));
        AppendIrInstruction(codes, CreateIrInstruction(kIrGoto, label_false));
        return codes;
    }
}

/* 表达式按从左到右求值，参数节点插入表头，使最终 ARG 按逆序输出。 */
static IrList* IrTranslateArgs(SyntaxNode* node, ArgList* arg_list) {
    char* t1 = CreateIrTemporary();
    IrList* codes1 = IrTranslateExp(node->first_child, t1);

    ArgId* cur = malloc(sizeof(ArgId));
    cur->next = NULL;
    cur->operand = t1;
    if (arg_list->head == NULL) {
        arg_list->head = cur;
    } else {
        cur->next = arg_list->head;
        arg_list->head = cur;
    }
    if (!FindSyntaxChild(node, "COMMA")) {
        return codes1;
    }
    IrList* codes2 = IrTranslateArgs(FindSyntaxChild(node, "Args"), arg_list);
    AppendIrList(codes1, codes2);
    return codes1;
}

IrList* TranslateProgramToIr(SyntaxNode* program) {
    return IrTranslateExtDefList(program ? program->first_child : NULL);
}
