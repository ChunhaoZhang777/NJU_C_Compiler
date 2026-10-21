/* 语法树拥有节点和词素副本；分配链让成功及错误路径使用同一释放入口。 */
#include "syntax_tree.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SyntaxNode* g_syntax_root = NULL;
static SyntaxNode* g_allocated_nodes = NULL;

static void* AllocateSyntaxMemory(size_t size) {
    void* memory = calloc(1, size);
    if (memory == NULL) {
        fputs("Out of memory while building syntax tree.\n", stderr);
        exit(EXIT_FAILURE);
    }
    return memory;
}

static char* CopySyntaxText(const char* text) {
    size_t size = strlen(text) + 1;
    char* copy = AllocateSyntaxMemory(size);
    memcpy(copy, text, size);
    return copy;
}

SyntaxNode* CreateSyntaxNode(const char* name, const char* content, SyntaxNodeKind type, int line) {
    SyntaxNode* node = AllocateSyntaxMemory(sizeof(*node));
    node->allocation_next = g_allocated_nodes;
    g_allocated_nodes = node;
    node->lex_name = CopySyntaxText(name);
    node->content = CopySyntaxText(content);
    node->type = type;
    node->line = line;
    if (strcmp(name, "FLOAT") == 0) {
        sscanf(content, "%f", &node->float_value);
    } else if (strcmp(name, "INT") == 0) {
        node->int_value = (unsigned int)strtoul(content, NULL, 0);
    }
    return node;
}

void AddSyntaxChildren(SyntaxNode* parent, int count, ...) {
    assert(parent != NULL && parent->first_child == NULL);
    va_list arguments;
    va_start(arguments, count);
    SyntaxNode* previous = NULL;
    for (int i = 0; i < count; ++i) {
        SyntaxNode* child = va_arg(arguments, SyntaxNode*);
        if (child == NULL) {
            continue;
        }
        assert(child->next == NULL);
        child->parent = parent;
        if (previous == NULL) {
            parent->first_child = child;
            parent->line = child->line;
        } else {
            previous->next = child;
        }
        previous = child;
    }
    va_end(arguments);
}

SyntaxNode* FindSyntaxChild(SyntaxNode* parent, const char* name) {
    for (SyntaxNode* child = parent ? parent->first_child : NULL; child; child = child->next) {
        if (strcmp(child->lex_name, name) == 0) {
            return child;
        }
    }
    return NULL;
}

void SetSyntaxRoot(SyntaxNode* node) {
    g_syntax_root = node;
}

void PrintSyntaxTree(SyntaxNode* node, int depth) {
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < depth; ++i) {
        fputs("  ", stdout);
    }
    if (node->type == kNonterminalNode) {
        printf("%s (%d)\n", node->lex_name, node->line);
    } else if (strcmp(node->lex_name, "ID") == 0 || strcmp(node->lex_name, "TYPE") == 0) {
        printf("%s: %s\n", node->lex_name, node->content);
    } else if (strcmp(node->lex_name, "INT") == 0) {
        printf("INT: %u\n", node->int_value);
    } else if (strcmp(node->lex_name, "FLOAT") == 0) {
        printf("FLOAT: %f\n", node->float_value);
    } else {
        printf("%s\n", node->lex_name);
    }
    PrintSyntaxTree(node->first_child, depth + 1);
    PrintSyntaxTree(node->next, depth);
}

void ReleaseSyntaxTree(void) {
    while (g_allocated_nodes != NULL) {
        SyntaxNode* next = g_allocated_nodes->allocation_next;
        free(g_allocated_nodes->lex_name);
        free(g_allocated_nodes->content);
        free(g_allocated_nodes);
        g_allocated_nodes = next;
    }
    g_syntax_root = NULL;
}
