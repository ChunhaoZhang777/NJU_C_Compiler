/* 命令行驱动：只有当前阶段成功，才继续分析或创建输出文件。 */
#include <stdio.h>

#include "frontend.h"
#include "ir.h"
#include "ir_translate.h"
#include "mips.h"
#include "semantic.h"
#include "syntax_tree.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.cmm output.s\n", argv[0]);
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
    DefineBuiltinFunctions();
    if (AnalyzeProgram(g_syntax_root)) {
        ReleaseSyntaxTree();
        return 1;
    }
    IrList* codes = TranslateProgramToIr(g_syntax_root);
    MipsList* assembly = TranslateIrToMips(codes);
    FILE* output = fopen(argv[2], "w");
    if (output == NULL) {
        perror(argv[2]);
        ReleaseSyntaxTree();
        return 1;
    }
    PrintMips(output, assembly);
    int write_failed = ferror(output);
    if (fclose(output) != 0)
        write_failed = 1;
    ReleaseSyntaxTree();
    if (write_failed) {
        fprintf(stderr, "Failed to write %s\n", argv[2]);
        return 1;
    }
    return 0;
}
