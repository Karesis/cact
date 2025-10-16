#include <stdlib.h>
#include "error.h" // 用于错误报告
#include "parser.h"

// 定义在 type.h 中声明的全局类型变量
Type *ty_void;
Type *ty_bool;
Type *ty_int;
Type *ty_float;
Type *ty_double;

// 一个创建新类型对象的内部辅助函数
static Type *new_type(TypeKind kind, int size, int align) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

// 初始化类型系统
void types_init(void) {
    ty_void = new_type(TY_VOID, 0, 0);
    ty_bool = new_type(TY_BOOL, 1, 1);
    ty_int = new_type(TY_INT, 4, 4);
    ty_float = new_type(TY_FLOAT, 4, 4);
    ty_double = new_type(TY_DOUBLE, 8, 8);
}

// 创建数组类型
Type *array_of(Type *base, int len) {
    Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
    ty->base = base;
    ty->array_len = len;
    return ty;
}

// 创建函数类型
Type *func_type(Type *return_ty) {
    Type *ty = new_type(TY_FUNC, 8, 8); // 函数在机器码层面可以看作一个指针
    ty->return_ty = return_ty;
    return ty;
}

// 为 AST 节点添加类型信息（核心语义分析）
void add_type(Node *node) {
    if (!node || node->ty)
        return;

    // 递归地为子节点添加类型信息
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->index);

    for (Node *n = node->body; n; n = n->next)
        add_type(n);
    for (Node *n = node->args; n; n = n->next)
        add_type(n);

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_NEG:
        // 对于算术运算，结果类型与左操作数类型相同
        node->ty = node->lhs->ty;
        return;
    case ND_ASSIGN:
        // 赋值表达式的类型与左操作数类型相同
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_LOG_AND:
    case ND_LOG_OR:
    case ND_LOG_NOT:
        // 关系和逻辑运算的结果总是布尔类型
        node->ty = ty_bool;
        return;
    case ND_VAR:
        // 变量节点的类型就是变量本身的类型
        node->ty = node->var->ty;
        return;
    case ND_NUM_INT:
        node->ty = ty_int;
        return;
    case ND_NUM_FLOAT:
        node->ty = ty_float;
        return;
    case ND_NUM_DOUBLE:
        node->ty = ty_double;
        return;
    case ND_BOOL:
        node->ty = ty_bool;
        return;
    case ND_FUNC_CALL:
        // 函数调用表达式的类型由函数签名中的返回类型决定
        // 这个信息在解析时已经被存入
        node->ty = node->var->ty->return_ty;
        return;
    case ND_ARRAY_ACCESS:
        // 数组访问表达式的类型是数组元素的基本类型
        if (node->lhs->ty->kind != TY_ARRAY)
            error("internal error: type of array access's lhs is not array");
        node->ty = node->lhs->ty->base;
        return;
    default:
        // 其他语句节点没有类型
        return;
    }
}
