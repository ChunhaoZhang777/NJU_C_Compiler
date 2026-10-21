#ifndef NJU_C_COMPILER_LAB1_CODE_SYNTAX_TREE_H_
#define NJU_C_COMPILER_LAB1_CODE_SYNTAX_TREE_H_

typedef enum {
    kNonterminalNode = 0x01,
    kTerminalNode = 0x02,
} SyntaxNodeKind;

typedef struct SyntaxNode {
    char* lex_name;
    char* content;
    SyntaxNodeKind type;
    int line;

    /* 首孩子、后继兄弟表示法；parent 仅用于回溯，不拥有子节点。 */
    struct SyntaxNode* parent;
    struct SyntaxNode* next;
    struct SyntaxNode* first_child;
    union {
        unsigned int int_value;
        float float_value;
    };
    /* 分配链独立于语法树，错误恢复丢弃的节点也能在编译结束时释放。 */
    struct SyntaxNode* allocation_next;
} SyntaxNode;

extern SyntaxNode* g_syntax_root;

/**
 * @brief 创建语法树节点并复制名称、词素。
 * @return 由树模块持有的节点，在 ReleaseSyntaxTree() 时统一释放。
 */
SyntaxNode* CreateSyntaxNode(const char* name, const char* content, SyntaxNodeKind type, int line);
/**
 * @brief 连接 count 个孩子，跳过空产生式，并沿用首个孩子的行号。
 * @note 每个可变参数都应为 SyntaxNode*；孩子通过后继兄弟指针连接。
 */
void AddSyntaxChildren(SyntaxNode* parent, int count, ...);
SyntaxNode* FindSyntaxChild(SyntaxNode* parent, const char* name);
void SetSyntaxRoot(SyntaxNode* node);
void PrintSyntaxTree(SyntaxNode* node, int depth);
void ReleaseSyntaxTree(void);

#endif  // NJU_C_COMPILER_LAB1_CODE_SYNTAX_TREE_H_
