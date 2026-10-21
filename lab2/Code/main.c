/* 命令行驱动：只有当前阶段成功，才继续分析或创建输出文件。 */
#include <stdio.h>

#include "frontend.h"
#include "semantic.h"
#include "syntax_tree.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.cmm\n", argv[0]);
        return 1;
    }
    FILE* input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(input);
    int parse_status = yyparse();
    fclose(input);
    yylex_destroy();
    if (parse_status || g_lex_error || g_syntax_error || g_syntax_root == NULL) {
        ReleaseSyntaxTree();
        return 1;
    }
    if (AnalyzeProgram(g_syntax_root)) {
        ReleaseSyntaxTree();
        return 1;
    }
    ReleaseSyntaxTree();
    return 0;
}
