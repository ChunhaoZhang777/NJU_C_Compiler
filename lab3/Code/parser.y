%code requires {
#include "syntax_tree.h"
}

%{
#include "frontend.h"

#include <stdio.h>

#include "syntax_tree.h"
int g_syntax_error = 0;
%}

%locations

%union {
    SyntaxNode* type_node;
}

%token <type_node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT TYPE LP
    RP LB RB LC RC STRUCT RETURN IF ELSE WHILE

%type <type_node> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier OptTag Tag VarDec
    FunDec VarList ParamDec CompSt StmtList Stmt DefList Def DecList Dec Exp Args

/* 从低到高声明优先级；取负使用独立 UMINUS，避免沿用二元减法优先级。 */
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT

/* 让 else 与最近一个尚未匹配 else 的 if 结合。 */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
/* 空产生式返回 NULL，不打印节点；error 分支保留同步点以继续报告后续错误。 */

Program : ExtDefList
    {
        $$ = CreateSyntaxNode("Program", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
        SetSyntaxRoot($$);
    }
    ;

ExtDefList : ExtDef ExtDefList
    {
        $$ = CreateSyntaxNode("ExtDefList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

ExtDef : Specifier ExtDecList SEMI
    {
        $$ = CreateSyntaxNode("ExtDef", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Specifier SEMI
    {
        $$ = CreateSyntaxNode("ExtDef", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | Specifier FunDec CompSt
    {
        $$ = CreateSyntaxNode("ExtDef", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | error SEMI
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Specifier FunDec error SEMI
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Specifier error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

ExtDecList : VarDec
    {
        $$ = CreateSyntaxNode("ExtDecList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | VarDec COMMA ExtDecList
    {
        $$ = CreateSyntaxNode("ExtDecList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | VarDec error ExtDecList
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | VarDec error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | error COMMA ExtDecList
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

Specifier : TYPE
    {
        $$ = CreateSyntaxNode("Specifier", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | StructSpecifier
    {
        $$ = CreateSyntaxNode("Specifier", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    ;

StructSpecifier : STRUCT OptTag LC DefList RC
    {
        $$ = CreateSyntaxNode("StructSpecifier", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 5, $1, $2, $3, $4, $5);
    }
    | STRUCT Tag
    {
        $$ = CreateSyntaxNode("StructSpecifier", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | STRUCT OptTag LC error RC
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

OptTag : ID
    {
        $$ = CreateSyntaxNode("OptTag", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

Tag : ID
    {
        $$ = CreateSyntaxNode("Tag", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    ;

VarDec : ID
    {
        $$ = CreateSyntaxNode("VarDec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | VarDec LB INT RB
    {
        $$ = CreateSyntaxNode("VarDec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 4, $1, $2, $3, $4);
    }
    | VarDec LB error RB
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | VarDec LB error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

FunDec : ID LP VarList RP
    {
        $$ = CreateSyntaxNode("FunDec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP
    {
        $$ = CreateSyntaxNode("FunDec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | error LP VarList RP
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | ID LP error RP
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

VarList : ParamDec COMMA VarList
    {
        $$ = CreateSyntaxNode("VarList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | ParamDec
    {
        $$ = CreateSyntaxNode("VarList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | error COMMA VarList
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

ParamDec : Specifier VarDec
    {
        $$ = CreateSyntaxNode("ParamDec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    ;

CompSt : LC DefList StmtList RC
    {
        $$ = CreateSyntaxNode("CompSt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 4, $1, $2, $3, $4);
    }
    | error RC
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

StmtList : Stmt StmtList
    {
        $$ = CreateSyntaxNode("StmtList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

Stmt : Exp SEMI
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | CompSt
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | RETURN Exp SEMI
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 5, $1, $2, $3, $4, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 7, $1, $2, $3, $4, $5, $6, $7);
    }
    | WHILE LP Exp RP Stmt
    {
        $$ = CreateSyntaxNode("Stmt", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 5, $1, $2, $3, $4, $5);
    }
    | Exp error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | RETURN Exp error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | error SEMI
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | RETURN error SEMI
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

DefList : Def DefList
    {
        $$ = CreateSyntaxNode("DefList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

Def : Specifier DecList SEMI
    {
        $$ = CreateSyntaxNode("Def", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Specifier error SEMI
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

DecList : Dec
    {
        $$ = CreateSyntaxNode("DecList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | Dec COMMA DecList
    {
        $$ = CreateSyntaxNode("DecList", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Dec error DecList
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Dec error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

Dec : VarDec
    {
        $$ = CreateSyntaxNode("Dec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | VarDec ASSIGNOP Exp
    {
        $$ = CreateSyntaxNode("Dec", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | VarDec ASSIGNOP error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

Exp : Exp ASSIGNOP Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp AND Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp OR Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp RELOP Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp PLUS Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp MINUS Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp STAR Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp DIV Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | LP Exp RP
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | MINUS Exp %prec UMINUS
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | NOT Exp
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 2, $1, $2);
    }
    | ID LP Args RP
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp LB Exp RB
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 4, $1, $2, $3, $4);
    }
    | Exp DOT ID
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | ID
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | INT
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | FLOAT
    {
        $$ = CreateSyntaxNode("Exp", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    | Exp ASSIGNOP error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp AND error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp OR error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp RELOP error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp PLUS error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp MINUS error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp STAR error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp DIV error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | LP error RP
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | MINUS error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | NOT error
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | ID LP error RP
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    | Exp LB error RB
    {
        $$ = NULL;
        g_syntax_error = 1;
    }
    ;

Args : Exp COMMA Args
    {
        $$ = CreateSyntaxNode("Args", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 3, $1, $2, $3);
    }
    | Exp
    {
        $$ = CreateSyntaxNode("Args", "", kNonterminalNode, @$.first_line);
        AddSyntaxChildren($$, 1, $1);
    }
    ;

%%

int yyerror(const char* msg) {
    /* 词法阶段已报告的同一行错误不再重复输出语法诊断。 */
    g_syntax_error = 1;
    if (g_last_error_line != yylineno) {
        printf("Error type B at Line %d: %s.\n", yylineno, msg);
        g_last_error_line = yylineno;
    }
    return 0;
}
