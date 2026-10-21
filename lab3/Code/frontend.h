#ifndef NJU_C_COMPILER_LAB3_CODE_FRONTEND_H_
#define NJU_C_COMPILER_LAB3_CODE_FRONTEND_H_

#include <stdio.h>

/* 单次编译的前端状态。词法、语法出错后，驱动程序停止后续阶段。 */
extern int g_lex_error;
extern int g_syntax_error;
extern int g_last_error_line;
extern int yylineno;

int yylex(void);
int yyparse(void);
int yyerror(const char* message);
void yyrestart(FILE* input);
int yylex_destroy(void);

#endif  // NJU_C_COMPILER_LAB3_CODE_FRONTEND_H_
