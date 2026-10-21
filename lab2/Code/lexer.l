%{
#include "frontend.h"

#include "parser.tab.h"
#include "syntax_tree.h"
static int g_scanner_column = 1;
int g_lex_error = 0;
int g_last_error_line = 0;
static int g_comment_start_line = 0;
#define YY_USER_ACTION                                  \
    yylloc.first_line = yylloc.last_line = yylineno;    \
    yylloc.first_column = g_scanner_column;             \
    yylloc.last_column = g_scanner_column + yyleng - 1; \
    g_scanner_column += yyleng;
%}

%option yylineno noyywrap noinput nounput never-interactive
%x COMMENT
DIGIT [0-9]
LETTER [a-zA-Z]

/* 有效数字规则先于 BADNUMBER，等长匹配时优先识别合法词法单元。 */
INT 0|([1-9][0-9]*)|0[0-7]+|0[xX][0-9a-fA-F]+
FLOAT {DIGIT}+"."{DIGIT}+|({DIGIT}+"."{DIGIT}*|"."{DIGIT}+)[eE][+-]?{DIGIT}+
BADNUMBER {DIGIT}+(\.{DIGIT}*)?([eE][+-]?[a-zA-Z0-9_]*)?|0[xX][a-zA-Z0-9_]*|\.{DIGIT}+[eE][+-]?[a-zA-Z0-9_]*
ID (_|{LETTER})(_|{LETTER}|{DIGIT})*
SEMI ";"
COMMA ","
ASSIGNOP "="
RELOP ">"|"<"|">="|"<="|"=="|"!="
PLUS "+"
MINUS "-"
STAR "*"
DIV "/"
AND "&&"
OR "||"
DOT "."
NOT "!"
TYPE "int"|"float"
LP "("
RP ")"
LB "["
RB "]"
LC "{"
RC "}"
STRUCT "struct"
RETURN "return"
IF "if"
ELSE "else"
WHILE "while"

SPACE " "|"\t"|"\r"
NLINE "\n"

LCOM "/*"
OCOM "//"

%%

{NLINE} { g_scanner_column = 1; }

{SPACE} { /* 空白只影响位置，不产生词法单元。 */ }

{OCOM}[^\n]* { /* 行注释允许直接在 EOF 结束。 */ }
    /* 块注释由独占状态处理，EOF 时仍可给出起始行号。 */
{LCOM} {
    g_comment_start_line = yylineno;
    BEGIN(COMMENT);
}
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>\n { g_scanner_column = 1; }
<COMMENT>. { /* Skip comment text. */ }
<COMMENT><<EOF>> {
    printf("Error type A at Line %d: Missing '*/'.\n", g_comment_start_line);
    g_lex_error = 1;
    g_last_error_line = yylineno;
    BEGIN(INITIAL);
    return 0;
}

{FLOAT} {
    yylval.type_node = CreateSyntaxNode("FLOAT", yytext, kTerminalNode, yylineno);
    return FLOAT;
}

{INT} {
    yylval.type_node = CreateSyntaxNode("INT", yytext, kTerminalNode, yylineno);
    return INT;
}

{BADNUMBER} {
    printf("Error type A at Line %d: Invalid number '%s'.\n", yylineno, yytext);
    g_last_error_line = yylineno;
    g_lex_error = 1;
}

{SEMI} {
    yylval.type_node = CreateSyntaxNode("SEMI", "", kTerminalNode, yylineno);
    return SEMI;
}

{COMMA} {
    yylval.type_node = CreateSyntaxNode("COMMA", "", kTerminalNode, yylineno);
    return COMMA;
}

{ASSIGNOP} {
    yylval.type_node = CreateSyntaxNode("ASSIGNOP", "", kTerminalNode, yylineno);
    return ASSIGNOP;
}

{RELOP} {
    yylval.type_node = CreateSyntaxNode("RELOP", yytext, kTerminalNode, yylineno);
    return RELOP;
}

{PLUS} {
    yylval.type_node = CreateSyntaxNode("PLUS", "", kTerminalNode, yylineno);
    return PLUS;
}

{MINUS} {
    yylval.type_node = CreateSyntaxNode("MINUS", "", kTerminalNode, yylineno);
    return MINUS;
}

{STAR} {
    yylval.type_node = CreateSyntaxNode("STAR", "", kTerminalNode, yylineno);
    return STAR;
}

{DIV} {
    yylval.type_node = CreateSyntaxNode("DIV", "", kTerminalNode, yylineno);
    return DIV;
}

{AND} {
    yylval.type_node = CreateSyntaxNode("AND", "", kTerminalNode, yylineno);
    return AND;
}

{OR} {
    yylval.type_node = CreateSyntaxNode("OR", "", kTerminalNode, yylineno);
    return OR;
}

{DOT} {
    yylval.type_node = CreateSyntaxNode("DOT", "", kTerminalNode, yylineno);
    return DOT;
}

{NOT} {
    yylval.type_node = CreateSyntaxNode("NOT", "", kTerminalNode, yylineno);
    return NOT;
}

{TYPE} {
    yylval.type_node = CreateSyntaxNode("TYPE", yytext, kTerminalNode, yylineno);
    return TYPE;
}

{LP} {
    yylval.type_node = CreateSyntaxNode("LP", "", kTerminalNode, yylineno);
    return LP;
}

{RP} {
    yylval.type_node = CreateSyntaxNode("RP", "", kTerminalNode, yylineno);
    return RP;
}

{LB} {
    yylval.type_node = CreateSyntaxNode("LB", "", kTerminalNode, yylineno);
    return LB;
}

{RB} {
    yylval.type_node = CreateSyntaxNode("RB", "", kTerminalNode, yylineno);
    return RB;
}

{LC} {
    yylval.type_node = CreateSyntaxNode("LC", "", kTerminalNode, yylineno);
    return LC;
}

{RC} {
    yylval.type_node = CreateSyntaxNode("RC", "", kTerminalNode, yylineno);
    return RC;
}

{STRUCT} {
    yylval.type_node = CreateSyntaxNode("STRUCT", "", kTerminalNode, yylineno);
    return STRUCT;
}

{RETURN} {
    yylval.type_node = CreateSyntaxNode("RETURN", "", kTerminalNode, yylineno);
    return RETURN;
}

{IF} {
    yylval.type_node = CreateSyntaxNode("IF", "", kTerminalNode, yylineno);
    return IF;
}

{ELSE} {
    yylval.type_node = CreateSyntaxNode("ELSE", "", kTerminalNode, yylineno);
    return ELSE;
}

{WHILE} {
    yylval.type_node = CreateSyntaxNode("WHILE", "", kTerminalNode, yylineno);
    return WHILE;
}

{ID} {
    yylval.type_node = CreateSyntaxNode("ID", yytext, kTerminalNode, yylineno);
    return ID;
}

. {
    printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);
    g_last_error_line = yylineno;
    g_lex_error = 1;
}

%%
