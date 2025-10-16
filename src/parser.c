#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "error.h"
#include "hash.h"

// --- Parser 状态 ---
static Token *current_token;
typedef struct Scope Scope;
struct Scope {
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
static Node *assign(void);
static Node *logor(void);
static Node *logand(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// --- 作用域和符号管理 ---
static void enter_scope(void) {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->next = scope;
    scope = sc;
}

static void leave_scope(void) {
    scope = scope->next;
}

static Obj *find_var(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->next) {
        Obj *var = hashmap_get2(&sc->vars, tok->loc, tok->len);
        if (var)
            return var;
    }
    return NULL;
}

static void push_var(Obj *var) {
    hashmap_put( &scope->vars, var->name, var);
}

static Obj *new_var(char *name, Type *ty, bool is_const) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->is_const = is_const;
    return var;
}

static Obj *new_lvar(char *name, Type *ty, bool is_const) {
    Obj *var = new_var(name, ty, is_const);
    var->is_local = true;
    if (current_fn) {
        var->next = current_fn->locals;
        current_fn->locals = var;
    }
    push_var(var);
    return var;
}

static Obj *new_gvar(char *name, Type *ty, bool is_const) {
    Obj *var = new_var(name, ty, is_const);
    var->next = globals;
    globals = var;
    push_var(var);
    return var;
}

// --- Parser 辅助函数 ---

// 查看当前 Token
static Token *peek(void) {
    return current_token;
}

// 消耗当前 Token 并前进到下一个
static Token *consume_tok(void) {
    Token *tok = current_token;
    current_token = current_token->next;
    return tok;
}

// 检查当前 token 是否为预期类型，如果是则消耗它并返回 true
static bool consume(TokenKind kind) {
    if (peek()->kind == kind) {
        consume_tok();
        return true;
    }
    return false;
}

// 检查当前 token 是否为预期类型，如果是则消耗它，否则报错
static void expect(TokenKind kind) {
    if (peek()->kind != kind)
        error_tok(peek(), "expected token %d, but got %d", kind, peek()->kind);
    consume_tok();
}

// 提取标识符的名字
static char *get_ident(void) {
    if (peek()->kind != TK_IDENT)
        error_tok(peek(), "expected an identifier");
    Token *tok = consume_tok();
    char *name = calloc(1, tok->len + 1);
    strncpy(name, tok->loc, tok->len);
    return name;
}

// 解析一个基本类型 (不消耗 token)
static Type* peek_btype() {
    TokenKind kind = peek()->kind;
    if (kind == TK_INT) return ty_int;
    if (kind == TK_FLOAT) return ty_float;
    if (kind == TK_DOUBLE) return ty_double;
    if (kind == TK_BOOL) return ty_bool;
    return NULL;
}

// 解析一个基本类型 (消耗 token)
static Type* consume_btype() {
    Type *ty = peek_btype();
    if(ty) consume_tok();
    return ty;
}

// 判断当前 token 是否是类型说明符的开始
static bool is_typename(void) {
    return peek_btype() || peek()->kind == TK_CONST || peek()->kind == TK_VOID;
}


// --- 递归下降解析函数 ---

static Node *primary() {
    Token *tok;

    if (consume(TK_L_PAREN)) {
        Node *node = expr();
        expect(TK_R_PAREN);
        return node;
    }

    tok = peek();
    if (consume(TK_NUM_INT))   return new_num_int(tok->val.i_val, tok);
    if (consume(TK_NUM_FLOAT)) return new_num_float(tok->val.f_val, tok);
    if (consume(TK_NUM_DOUBLE))return new_num_double(tok->val.d_val, tok);
    if (consume(TK_LIT_BOOL))  return new_bool(tok->val.b_val, tok);

    tok = peek();
    if (tok->kind == TK_IDENT) {
        Token *name_tok = consume_tok();
        
        if (peek()->kind == TK_L_PAREN) {
            consume_tok(); // consume '('
            Node *node = new_node(ND_FUNC_CALL, name_tok);
            node->func_name = strndup(name_tok->loc, name_tok->len);
            
            Obj* fn = find_var(name_tok);
            if (!fn || !fn->is_function)
                error_tok(name_tok, "function '%s' not declared", node->func_name);
            node->var = fn;
            
            Node head = {};
            Node *cur = &head;
            if (!consume(TK_R_PAREN)) {
                do {
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
        
        while(peek()->kind == TK_L_BRACKET) {
            Token* start_tok = consume_tok();
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

static Node *unary() {
    Token* tok;
    if (consume(TK_PLUS))
        return primary();
    tok = peek();
    if (consume(TK_MINUS))
        return new_unary(ND_NEG, primary(), tok);
    if (consume(TK_LOG_NOT))
        return new_unary(ND_LOG_NOT, primary(), tok);
    return primary();
}

static Node *mul() {
    Node *node = unary();
    for (;;) {
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

static Node *add() {
    Node *node = mul();
    for (;;) {
        Token *tok = peek();
        if (consume(TK_PLUS))
            node = new_binary(ND_ADD, node, mul(), tok);
        else if (consume(TK_MINUS))
            node = new_binary(ND_SUB, node, mul(), tok);
        else
            return node;
    }
}

static Node *relational() {
    Node *node = add();
    for (;;) {
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

static Node *equality() {
    Node *node = relational();
    for (;;) {
        Token *tok = peek();
        if (consume(TK_EQ))
            node = new_binary(ND_EQ, node, relational(), tok);
        else if (consume(TK_NEQ))
            node = new_binary(ND_NE, node, relational(), tok);
        else
            return node;
    }
}

static Node *logand() {
    Node *node = equality();
    for(;;) {
        Token *tok = peek();
        if (consume(TK_LOG_AND))
            node = new_binary(ND_LOG_AND, node, equality(), tok);
        else
            return node;
    }
}

static Node *logor() {
    Node *node = logand();
    for(;;) {
        Token *tok = peek();
        if (consume(TK_LOG_OR))
            node = new_binary(ND_LOG_OR, node, logand(), tok);
        else
            return node;
    }
}

static Node *assign() {
    Node *node = logor();
    Token *tok = peek();
    if (consume(TK_ASSIGN))
        node = new_binary(ND_ASSIGN, node, assign(), tok);
    return node;
}

static Node *expr() {
    return assign();
}

static Node *stmt() {
    Token *tok = peek();
    if (consume(TK_IF)) {
        Node *node = new_node(ND_IF, tok);
        expect(TK_L_PAREN);
        node->cond = expr();
        expect(TK_R_PAREN);
        node->then = stmt();
        if (consume(TK_ELSE))
            node->els = stmt();
        return node;
    }

    if (consume(TK_WHILE)) {
        Node *node = new_node(ND_WHILE, tok);
        expect(TK_L_PAREN);
        node->cond = expr();
        expect(TK_R_PAREN);
        node->then = stmt();
        return node;
    }

    if (consume(TK_RETURN)) {
        Node *node = new_node(ND_RETURN, tok);
        if(!consume(TK_SEMICOLON)) {
            node->lhs = expr();
            expect(TK_SEMICOLON);
        }
        return node;
    }

    if (consume(TK_L_BRACE)) {
        Node *node = new_node(ND_BLOCK, tok);
        Node head = {};
        Node *cur = &head;
        enter_scope();
        while (!consume(TK_R_BRACE)) {
            if (is_typename()) {
                cur->next = declaration();
            } else {
                cur->next = stmt();
            }
            cur = cur->next;
        }
        leave_scope();
        node->body = head.next;
        return node;
    }

    if (consume(TK_BREAK)) {
        expect(TK_SEMICOLON);
        return new_node(ND_BREAK, tok);
    }

    if (consume(TK_CONTINUE)) {
        expect(TK_SEMICOLON);
        return new_node(ND_CONTINUE, tok);
    }

    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr();
    expect(TK_SEMICOLON);
    return node;
}

static Node *declaration() {
    Token *tok = peek();
    bool is_const = consume(TK_CONST);

    Type *ty = consume_btype();
    if (!ty)
        error_tok(tok, "expected a type specifier");

    // TODO: This only supports one declaration per line for now.
    char *name = get_ident();

    while(consume(TK_L_BRACKET)) {
        Token* len_tok = peek();
        if(!consume(TK_NUM_INT))
            error_tok(len_tok, "array size must be an integer constant");
        ty = array_of(ty, len_tok->val.i_val);
        expect(TK_R_BRACKET);
    }
    
    Obj *var;
    if (current_fn)
        var = new_lvar(name, ty, is_const);
    else
        var = new_gvar(name, ty, is_const);

    if (consume(TK_ASSIGN)) {
        Node *node = new_binary(ND_ASSIGN, new_var_node(var, tok), expr(), tok);
        expect(TK_SEMICOLON);
        return new_unary(ND_EXPR_STMT, node, tok);
    }

    expect(TK_SEMICOLON);
    return new_node(ND_EXPR_STMT, tok);
}

static void function() {
    Token *tok = peek();
    Type *return_ty = ty_void;
    if(peek()->kind != TK_VOID) {
        return_ty = consume_btype();
    } else {
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
    Obj  o_head = {};
    Obj *o_cur = &o_head;

    if (!consume(TK_R_PAREN)) {
        do {
            Type* param_ty = consume_btype();
            char* param_name = get_ident();
            while(consume(TK_L_BRACKET)) {
                if(!consume(TK_R_BRACKET)) { // Optional size for params
                    expect(TK_NUM_INT);
                    expect(TK_R_BRACKET);
                }
                param_ty = array_of(param_ty, -1);
            }

            p_cur = p_cur->next = param_ty;
            o_cur = o_cur->next = new_lvar(param_name, param_ty, false);
        } while (consume(TK_COMMA));
        expect(TK_R_PAREN);
    }
    
    fn->ty->params = p_head.next;
    fn->params = o_head.next;
    
    expect(TK_L_BRACE);

    Node head = {};
    Node *cur = &head;

    while(!consume(TK_R_BRACE)) {
        if (is_typename()) {
            cur->next = declaration();
        } else {
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
static bool is_function(void) {
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
    while (tok->kind == TK_L_BRACKET) {
        // 这是一个简化的向前看，我们假设括号是匹配的
        tok = tok->next;
        if(tok->kind == TK_NUM_INT) tok = tok->next; // skip size
        if (tok->kind != TK_R_BRACKET) return false;
        tok = tok->next;
    }

    // 最终判断
    return tok->kind == TK_L_PAREN;
}

Obj *parse(Token *tok) {
    globals = NULL;
    current_token = tok;
    
    types_init();
    enter_scope(); // Global scope

    while (peek()->kind != TK_EOF) {
        if (is_function()) {
            function();
        } else {
            declaration();
        }
    }
    
    leave_scope();
    return globals;
}

