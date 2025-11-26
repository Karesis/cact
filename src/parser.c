#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "error.h"
#include "hash.h"

// --- Parser 状态 ---
static Token *current_token;
typedef struct Scope Scope;
struct Scope
{
    Scope *next;
    HashMap vars;
};
static Scope *scope = &(Scope){};
static Obj *globals;
static Obj *current_fn;

// --- 前向声明 ---
static void function(void);
static Node *declaration(void);
static Node *stmt(void);
static Node *expr(void);
static Node *const_expr(void);
static Node *assign(void);
static Node *logor(void);
static Node *logand(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);
static Node *initializer_list(Token *tok);

static void add_builtin_function(char *name, Type *return_ty, Type *params_ty);

// --- 作用域和符号管理 ---
static void enter_scope(void)
{
    Scope *sc = calloc(1, sizeof(Scope));
    sc->next = scope;
    scope = sc;
}

static void leave_scope(void)
{
    scope = scope->next;
}

static Obj *find_var(Token *tok)
{
    for (Scope *sc = scope; sc; sc = sc->next)
    {
        Obj *var = hashmap_get2(&sc->vars, tok->loc, tok->len);
        if (var)
            return var;
    }
    return NULL;
}

static void push_var(Obj *var)
{
    hashmap_put(&scope->vars, var->name, var);
}

static Obj *new_var(char *name, Type *ty, bool is_const)
{
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->is_const = is_const;
    return var;
}

static Obj *new_lvar(char *name, Type *ty, bool is_const)
{
    Obj *var = new_var(name, ty, is_const);
    var->is_local = true;
    if (current_fn)
    {
        var->next = current_fn->locals;
        current_fn->locals = var;
    }
    push_var(var);
    return var;
}

// 这是一个只创建 Obj 并将其添加到当前作用域的辅助函数
// 它不处理 locals 链表
static Obj *new_param(char *name, Type *ty)
{
    Obj *var = new_var(name, ty, false); // is_const is false for params
    var->is_local = true;
    // 注意：这里不再有修改 current_fn->locals 的代码
    push_var(var);
    return var;
}

static Obj *new_gvar(char *name, Type *ty, bool is_const)
{
    Obj *var = new_var(name, ty, is_const);
    var->next = globals;
    globals = var;
    push_var(var);
    return var;
}

// --- Parser 辅助函数 ---

// 查看当前 Token
static Token *peek(void)
{
    return current_token;
}

// 消耗当前 Token 并前进到下一个
static Token *consume_tok(void)
{
    Token *tok = current_token;
    current_token = current_token->next;
    return tok;
}

// 检查当前 token 是否为预期类型，如果是则消耗它并返回 true
static bool consume(TokenKind kind)
{
    if (peek()->kind == kind)
    {
        consume_tok();
        return true;
    }
    return false;
}

// 检查当前 token 是否为预期类型，如果是则消耗它，否则报错
static void expect(TokenKind kind)
{
    if (peek()->kind != kind)
        error_tok(peek(), "expected token %d, but got %d", kind, peek()->kind);
    consume_tok();
}

// 提取标识符的名字
static char *get_ident(void)
{
    if (peek()->kind != TK_IDENT)
        error_tok(peek(), "expected an identifier");
    Token *tok = consume_tok();
    char *name = calloc(1, tok->len + 1);
    strncpy(name, tok->loc, tok->len);
    return name;
}

// 解析一个基本类型 (不消耗 token)
static Type *peek_btype()
{
    TokenKind kind = peek()->kind;
    if (kind == TK_INT)
        return ty_int;
    if (kind == TK_FLOAT)
        return ty_float;
    if (kind == TK_DOUBLE)
        return ty_double;
    if (kind == TK_BOOL)
        return ty_bool;
    return NULL;
}

// 解析一个基本类型 (消耗 token)
static Type *consume_btype()
{
    Type *ty = peek_btype();
    if (ty)
        consume_tok();
    return ty;
}

// 判断当前 token 是否是类型说明符的开始
static bool is_typename(void)
{
    return peek_btype() || peek()->kind == TK_CONST || peek()->kind == TK_VOID;
}

// --- 递归下降解析函数 ---

static Node *initializer_list(Token *tok)
{
    // 创建一个新的节点来代表整个初始化列表
    Node *node = new_node(ND_INIT_LIST, tok);
    expect(TK_L_BRACE);

    Node head = {};
    Node *cur = &head;

    // 如果不是空列表 `{}`, 就开始解析
    if (peek()->kind != TK_R_BRACE)
    {
        do
        {
            // 解析列表中的每一个元素
            // 用 assign() 可以解析 '1'，'a+b' 等各种表达式
            cur->next = assign();
            cur = cur->next;
        } while (consume(TK_COMMA));
    }

    expect(TK_R_BRACE);

    // 将链表挂到新节点的 body 或其他字段上
    node->body = head.next;
    return node;
}

static Node *primary()
{
    Token *tok;

    if (consume(TK_L_PAREN))
    {
        Node *node = expr();
        expect(TK_R_PAREN);
        return node;
    }

    tok = peek();
    if (consume(TK_NUM_INT))
        return new_num_int(tok->val.i_val, tok);
    if (consume(TK_NUM_FLOAT))
        return new_num_float(tok->val.f_val, tok);
    if (consume(TK_NUM_DOUBLE))
        return new_num_double(tok->val.d_val, tok);
    if (consume(TK_LIT_BOOL))
        return new_bool(tok->val.b_val, tok);

    tok = peek();
    if (tok->kind == TK_IDENT)
    {
        Token *name_tok = consume_tok();

        if (peek()->kind == TK_L_PAREN)
        {
            consume_tok(); // consume '('
            Node *node = new_node(ND_FUNC_CALL, name_tok);
            node->func_name = strndup(name_tok->loc, name_tok->len);

            Obj *fn = find_var(name_tok);
            if (!fn || !fn->is_function)
                error_tok(name_tok, "function '%s' not declared", node->func_name);
            node->var = fn;

            Node head = {};
            Node *cur = &head;
            if (!consume(TK_R_PAREN))
            {
                do
                {
                    cur->next = assign();
                    cur = cur->next;
                } while (consume(TK_COMMA));
                expect(TK_R_PAREN);
            }
            node->args = head.next;
            return node;
        }

        Obj *var = find_var(name_tok);
        if (!var)
            error_tok(name_tok, "variable '%.*s' not declared", name_tok->len, name_tok->loc);

        Node *node = new_var_node(var, name_tok);

        while (peek()->kind == TK_L_BRACKET)
        {
            Token *start_tok = consume_tok();
            Node *parent = new_node(ND_ARRAY_ACCESS, start_tok);
            parent->lhs = node;
            parent->index = expr();
            node = parent;
            expect(TK_R_BRACKET);
        }
        return node;
    }

    error_tok(tok, "expected an expression");
    return NULL; // unreachable
}

static Node *unary()
{
    Token *tok;
    if (consume(TK_PLUS))
        return primary();
    tok = peek();
    if (consume(TK_MINUS))
        return new_unary(ND_NEG, primary(), tok);
    if (consume(TK_LOG_NOT))
        return new_unary(ND_LOG_NOT, primary(), tok);
    return primary();
}

static Node *mul()
{
    Node *node = unary();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_STAR))
            node = new_binary(ND_MUL, node, unary(), tok);
        else if (consume(TK_SLASH))
            node = new_binary(ND_DIV, node, unary(), tok);
        else if (consume(TK_PERCENT))
            node = new_binary(ND_MOD, node, unary(), tok);
        else
            return node;
    }
}

static Node *add()
{
    Node *node = mul();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_PLUS))
            node = new_binary(ND_ADD, node, mul(), tok);
        else if (consume(TK_MINUS))
            node = new_binary(ND_SUB, node, mul(), tok);
        else
            return node;
    }
}

static Node *relational()
{
    Node *node = add();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_LT))
            node = new_binary(ND_LT, node, add(), tok);
        else if (consume(TK_LE))
            node = new_binary(ND_LE, node, add(), tok);
        else if (consume(TK_GT))
            node = new_binary(ND_LT, add(), node, tok);
        else if (consume(TK_GE))
            node = new_binary(ND_LE, add(), node, tok);
        else
            return node;
    }
}

static Node *equality()
{
    Node *node = relational();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_EQ))
            node = new_binary(ND_EQ, node, relational(), tok);
        else if (consume(TK_NEQ))
            node = new_binary(ND_NE, node, relational(), tok);
        else
            return node;
    }
}

static Node *logand()
{
    Node *node = equality();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_LOG_AND))
            node = new_binary(ND_LOG_AND, node, equality(), tok);
        else
            return node;
    }
}

static Node *logor()
{
    Node *node = logand();
    for (;;)
    {
        Token *tok = peek();
        if (consume(TK_LOG_OR))
            node = new_binary(ND_LOG_OR, node, logand(), tok);
        else
            return node;
    }
}

static Node *assign()
{
    Node *node = logor();
    Token *tok = peek();
    if (consume(TK_ASSIGN))
    {
        // 修改点：右侧不再是 assign()，而是一个不允许赋值的表达式
        // 这就禁止了 a = b = c 的情况
        Node *rhs = logor();
        node = new_binary(ND_ASSIGN, node, rhs, tok);
    }
    return node;
}

static Node *expr()
{
    return assign();
}

// 一个新的、严格的解析函数
static Node *const_expr()
{
    Token *tok = peek();
    if (consume(TK_NUM_INT))
        return new_num_int(tok->val.i_val, tok);
    if (consume(TK_NUM_FLOAT))
        return new_num_float(tok->val.f_val, tok);
    if (consume(TK_NUM_DOUBLE))
        return new_num_double(tok->val.d_val, tok);
    if (consume(TK_LIT_BOOL))
        return new_bool(tok->val.b_val, tok);

    // 如果不是以上任何一种，就说明语法错误
    error_tok(tok, "expected a constant expression (literal value)");
    return NULL; // Unreachable
}

static Node *stmt()
{
    Token *tok = peek();
    if (consume(TK_IF))
    {
        Node *node = new_node(ND_IF, tok);
        expect(TK_L_PAREN);
        node->cond = expr();
        expect(TK_R_PAREN);
        node->then = stmt();
        if (consume(TK_ELSE))
            node->els = stmt();
        return node;
    }

    if (consume(TK_WHILE))
    {
        Node *node = new_node(ND_WHILE, tok);
        expect(TK_L_PAREN);
        node->cond = expr();
        expect(TK_R_PAREN);
        node->then = stmt();
        return node;
    }

    if (consume(TK_RETURN))
    {
        Node *node = new_node(ND_RETURN, tok);
        if (!consume(TK_SEMICOLON))
        {
            node->lhs = expr();
            expect(TK_SEMICOLON);
        }
        return node;
    }

    if (consume(TK_L_BRACE))
    {
        Node *node = new_node(ND_BLOCK, tok);
        Node head = {};
        Node *cur = &head;
        enter_scope();
        while (!consume(TK_R_BRACE))
        {
            if (is_typename())
            {
                cur->next = declaration();
            }
            else
            {
                cur->next = stmt();
            }
            cur = cur->next;
        }
        leave_scope();
        node->body = head.next;
        return node;
    }

    if (consume(TK_BREAK))
    {
        expect(TK_SEMICOLON);
        return new_node(ND_BREAK, tok);
    }

    if (consume(TK_CONTINUE))
    {
        expect(TK_SEMICOLON);
        return new_node(ND_CONTINUE, tok);
    }

    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr();
    expect(TK_SEMICOLON);
    return node;
}

static Node *declaration()
{
    Token *start_tok = peek();
    bool is_const = consume(TK_CONST);

    // 1. 先解析出所有变量共享的基础类型
    Type *base_ty = consume_btype();
    if (!base_ty)
        error_tok(start_tok, "expected a type specifier");

    // 2. 使用 "dummy head" 技巧来构建一个语句链表
    //    因为 `int i, j=5;` 实际上是两个独立的声明+赋值语句
    Node head = {};
    Node *cur = &head;
    int count = 0;

    // 3. 使用 do-while 循环来处理至少一个 VarDef
    do
    {
        // count > 0 意味着这不是第一个变量，前面必须有逗号
        if (count > 0)
        {
            expect(TK_COMMA);
        }

        // --- 以下是解析单个 VarDef 的逻辑 ---
        char *name = get_ident();
        Type *ty = base_ty; // 每个变量的类型都从基础类型开始

        // 处理数组部分
        while (consume(TK_L_BRACKET))
        {
            Token *len_tok = peek();
            if (!consume(TK_NUM_INT))
                error_tok(len_tok, "array size must be an integer constant");
            ty = array_of(ty, len_tok->val.i_val);
            expect(TK_R_BRACKET);
        }

        // 创建变量对象
        Obj *var;
        if (current_fn)
            var = new_lvar(name, ty, is_const);
        else
            var = new_gvar(name, ty, is_const);

        // 处理可选的初始化部分
        if (consume(TK_ASSIGN))
        {
            Node *rhs;
            if (peek()->kind == TK_L_BRACE)
            {
                rhs = initializer_list(peek());
            }
            else
            {
                rhs = const_expr();
            }
            Node *assign_node = new_binary(ND_ASSIGN, new_var_node(var, start_tok), rhs, start_tok);
            cur->next = new_unary(ND_EXPR_STMT, assign_node, start_tok);
            cur = cur->next;
        }
        else
        {
            cur->next = new_node(ND_EXPR_STMT, start_tok);
            cur = cur->next;
        }
        count++;

        // 4. 循环条件：如果下一个是逗号，就继续循环
    } while (peek()->kind == TK_COMMA);

    // 5. 循环结束后，期望一个分号
    expect(TK_SEMICOLON);

    // 6. 返回构建好的语句链表的头节点
    // 如果没有任何初始化语句，head.next 会是 NULL，也是正确的
    return head.next;
}

static void function()
{
    Token *tok = peek();
    Type *return_ty = ty_void;
    if (peek()->kind != TK_VOID)
    {
        return_ty = consume_btype();
    }
    else
    {
        consume_tok(); // consume "void"
    }

    char *name = get_ident();
    Type *ty = func_type(return_ty);

    Obj *fn = new_gvar(name, ty, false);
    fn->is_function = true;

    current_fn = fn;

    enter_scope();
    expect(TK_L_PAREN);

    Type p_head = {};
    Type *p_cur = &p_head;
    Obj o_head = {};
    Obj *o_cur = &o_head;

    if (!consume(TK_R_PAREN))
    {
        do
        {
            Type *param_ty = consume_btype();
            char *param_name = get_ident();
            // 1. 用 'if' 语句处理第一个可选的维度 ['[' IntConst? ']']
            if (consume(TK_L_BRACKET))
            {
                int len = -1; // -1 代表省略的长度

                // 检查方括号内是否为空
                if (peek()->kind == TK_NUM_INT)
                {
                    Token *len_tok = consume_tok();
                    len = len_tok->val.i_val;
                }
                expect(TK_R_BRACKET);
                param_ty = array_of(param_ty, len);

                // 2. 用 'while' 循环处理所有后续的、必须有长度的维度 {'[' IntConst ']'}
                while (consume(TK_L_BRACKET))
                {
                    Token *len_tok = peek();
                    // 根据规范，后续维度必须有长度
                    expect(TK_NUM_INT);
                    param_ty = array_of(param_ty, len_tok->val.i_val);
                    expect(TK_R_BRACKET);
                }
            }

            p_cur = p_cur->next = param_ty;
            o_cur = o_cur->next = new_param(param_name, param_ty);
        } while (consume(TK_COMMA));
        expect(TK_R_PAREN);
    }

    fn->ty->params = p_head.next;
    fn->params = o_head.next;

    expect(TK_L_BRACE);

    Node head = {};
    Node *cur = &head;

    while (!consume(TK_R_BRACE))
    {
        if (is_typename())
        {
            cur->next = declaration();
        }
        else
        {
            cur->next = stmt();
        }
        cur = cur->next;
    }

    fn->body = head.next;
    fn->locals = current_fn->locals;
    leave_scope();
    current_fn = NULL;
}

// 用向前看来判断是函数还是全局变量
// 语法: type-specifier declarator ...
// 我们需要向前看到 declarator 后面是不是'('
static bool is_function(void)
{
    Token *tok = current_token;

    // 跳过类型说明符 (const? btype)
    if (tok->kind == TK_CONST)
        tok = tok->next;
    if (tok->kind == TK_INT || tok->kind == TK_FLOAT || tok->kind == TK_DOUBLE || tok->kind == TK_BOOL || tok->kind == TK_VOID)
        tok = tok->next;
    else
        return false; // 不是一个顶层声明的开始

    // 跳过标识符
    if (tok->kind != TK_IDENT)
        return false;
    tok = tok->next;

    // 跳过可能的数组声明符 `[]`
    while (tok->kind == TK_L_BRACKET)
    {
        // 这是一个简化的向前看，我们假设括号是匹配的
        tok = tok->next;
        if (tok->kind == TK_NUM_INT)
            tok = tok->next; // skip size
        if (tok->kind != TK_R_BRACKET)
            return false;
        tok = tok->next;
    }

    // 最终判断
    return tok->kind == TK_L_PAREN;
}

Obj *parse(Token *tok)
{
    globals = NULL;
    current_token = tok;

    types_init();
    enter_scope(); // Global scope

    // 3. 预加载所有内置的运行时库函数

    // print_int(int)
    Type *p_int = ty_int;
    add_builtin_function("print_int", ty_void, p_int);

    // print_float(float)
    Type *p_float = ty_float;
    add_builtin_function("print_float", ty_void, p_float);

    // print_double(double)
    Type *p_double = ty_double;
    add_builtin_function("print_double", ty_void, p_double);

    // print_bool(bool)
    Type *p_bool = ty_bool;
    add_builtin_function("print_bool", ty_void, p_bool);

    // get_int() - no params
    add_builtin_function("get_int", ty_int, NULL);

    // get_float() - no params
    add_builtin_function("get_float", ty_float, NULL);

    // get_double() - no params
    add_builtin_function("get_double", ty_double, NULL);

    while (peek()->kind != TK_EOF)
    {
        if (is_function())
        {
            function();
        }
        else
        {
            declaration();
        }
    }

    leave_scope();
    return globals;
}

// 放在你的 new_gvar 或类似函数附近
// name: 函数名
// return_ty: 函数的返回类型
// params_ty: 一个 Type* 的链表，代表参数类型
static void add_builtin_function(char *name, Type *return_ty, Type *params_ty)
{
    // 1. 创建函数类型
    Type *ty = func_type(return_ty);
    ty->params = params_ty;

    // 2. 创建代表函数的 Obj (符号)
    //    这里的 is_const=false, is_local=false 都是默认值
    Obj *fn = new_gvar(name, ty, false);
    fn->is_function = true;

    // 注意：内置函数没有函数体(body)和局部变量(locals)，所以这些字段保持 NULL
}