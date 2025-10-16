#include <stdlib.h>
#include "parser.h" // 包含所有结构体定义

// --- AST 构造辅助函数的实现 ---

Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_unary(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

Node *new_num_int(int val, Token *tok) {
    Node *node = new_node(ND_NUM_INT, tok);
    node->i_val = val;
    return node;
}

Node *new_num_float(float val, Token *tok) {
    Node *node = new_node(ND_NUM_FLOAT, tok);
    node->f_val = val;
    return node;
}

Node *new_num_double(double val, Token *tok) {
    Node *node = new_node(ND_NUM_DOUBLE, tok);
    node->d_val = val;
    return node;
}

Node *new_bool(bool val, Token *tok) {
    Node *node = new_node(ND_BOOL, tok);
    node->b_val = val;
    return node;
}

Node *new_var_node(Obj *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

