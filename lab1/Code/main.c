/* Lab1 驱动：前端成功后打印语法树，结束时统一释放全部节点。 */
#include <stdio.h>

#include "frontend.h"
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
    int status = parse_status || g_lex_error || g_syntax_error || g_syntax_root == NULL;
    if (!status) {
        PrintSyntaxTree(g_syntax_root, 0);
    }
    ReleaseSyntaxTree();
    return status ? 1 : 0;
}
