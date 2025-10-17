#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "token.h" // 引入 Token 定义，因为 AST 节点会引用它

// 前向声明，解决结构体之间的循环引用
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Type Type;

// 类型种类的枚举
typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_INT,
    TY_FLOAT,
    TY_DOUBLE,
    TY_ARRAY,
    TY_FUNC,
} TypeKind;

// Type 结构体，用于表示 CACT 中的所有类型
struct Type {
    TypeKind kind;  // 类型的种类
    int size;       // 类型的大小 (in bytes)
    int align;      // 类型的对齐要求

    // 指针或数组的基本类型
    // 例如，对于 int a[10]，它的 kind 是 TY_ARRAY, base 是指向 TY_INT 的指针
    Type *base;

    // 数组的长度
    int array_len;

    // 函数签名
    Type *return_ty; // 函数返回类型
    Type *params;    // 函数参数类型列表 (链表)
    Type *next;      // 链表中的下一个参数
};

// --- 全局基本类型对象 ---
// 通过 extern 关键字，让其他文件可以访问这些在 type.c 中定义的全局变量
extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_int;
extern Type *ty_float;
extern Type *ty_double;


// --- 类型系统函数 ---

// 初始化类型系统，创建上述全局基本类型对象
void types_init(void);

// 创建一个数组类型
// base: 数组元素的基本类型
// len:  数组的长度
Type *array_of(Type *base, int len);

// 创建一个函数类型
// return_ty: 函数的返回类型
Type *func_type(Type *return_ty);

// --- 2. 对象/符号 (Objects/Symbols) ---
// ------------------------------------

// Obj 结构体，用于表示所有被命名的实体，如变量和函数
// 这本质上是符号表中的一个条目
struct Obj {
    Obj *next;      // 指向下一个对象，形成链表
    char *name;     // 对象的名字
    Type *ty;       // 对象的类型
    bool is_local;  // 是局部变量还是全局变量/函数

    // 局部变量
    int offset;     // 距离栈帧基地址的偏移量

    // 全局变量/函数
    bool is_function;
    bool is_const;

    // 函数
    Node *body;     // 函数体的 AST
    Obj *params;    // 函数的参数列表 (链表)
    Obj *locals;    // 函数内的局部变量列表 (链表)
    int stack_size; // 函数需要的栈大小
};


// --- 3. AST 节点 (AST Nodes) ---
// ------------------------------------

// AST 节点种类的枚举
typedef enum {
    // --- 表达式 (Expressions) ---
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_MOD,       // %
    ND_NEG,       // Unary -
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    // GT (>) and GE (>=) can be implemented by swapping operands of LT and LE
    ND_LOG_AND,   // &&
    ND_LOG_OR,    // ||
    ND_LOG_NOT,   // Unary !
    ND_ASSIGN,    // =
    ND_VAR,       // Variable access
    ND_NUM_INT,   // Integer literal
    ND_NUM_FLOAT, // Float literal
    ND_NUM_DOUBLE,// Double literal
    ND_BOOL,      // Boolean literal
    ND_INIT_LIST, // 代表 { ... } 初始化列表
    ND_FUNC_CALL, // Function call
    ND_ARRAY_ACCESS, // Array access, e.g., arr[i]

    // --- 语句 (Statements) ---
    ND_RETURN,    // "return"
    ND_IF,        // "if"
    ND_WHILE,     // "while"
    ND_BLOCK,     // { ... }
    ND_BREAK,     // "break"
    ND_CONTINUE,  // "continue"
    ND_EXPR_STMT, // Expression statement (e.g., i++;)

} NodeKind;

// AST 节点结构体
// 这是一个“大而全”的设计，一个节点只会使用其中的部分字段
struct Node {
    NodeKind kind; // 节点类型
    Node *next;    // 用于连接语句块中的下一条语句
    Token *tok;    // 节点对应的代表性 Token (用于报错)
    Type *ty;      // 节点表达式的类型 (在语义分析阶段填充)

    // 二元/一元操作符
    Node *lhs;     // Left-hand side
    Node *rhs;     // Right-hand side

    // "if" 语句
    Node *cond;
    Node *then;
    Node *els;     // 可选的 else 分支

    // "while" 循环
    // cond 和 then 字段可以复用
    
    // 语句块 { ... }
    Node *body;

    // 函数调用
    char *func_name;
    Node *args;    // 参数列表 (链表)

    // 变量或数组访问
    Obj *var;           // 指向变量的 Obj
    Node *index;        // 数组的索引表达式

    // 字面量
    int i_val;
    float f_val;
    double d_val;
    bool b_val;
};

Node *new_node(NodeKind kind, Token *tok);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_unary(NodeKind kind, Node *lhs, Token *tok);
Node *new_num_int(int val, Token *tok);
Node *new_num_float(float val, Token *tok);
Node *new_num_double(double val, Token *tok);
Node *new_bool(bool val, Token *tok);
Node *new_var_node(Obj *var, Token *tok);

// 为一个 AST 节点添加类型信息。
// 这是语义分析的核心函数之一，它会根据节点的种类和子节点的类型来推导当前节点的类型。
void add_type(Node *node);

// --- 主解析函数 ---
//
// 输入: 从词法分析器获得的 Token 链表的头节点
// 输出: 一个 Obj 链表，代表了程序中所有的全局变量和函数
Obj *parse(Token *tok);

#endif // PARSER_H
