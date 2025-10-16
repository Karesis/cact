#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "token.h"
#include "lexer.h"
#include "hash.h"
#include "str.h"

// Lexer的内部状态
static struct {
    str source;           // 源代码的字符串视图
    const char *filename; // 文件名，用于报错
    char *current;        // 指向当前正在扫描的字符
    int row;              // 当前行号
    int col;              // 当前列号
    Token *head;          // Token 链表的头
    Token *tail;          // Token 链表的尾，用于O(1)追加
} L;

// 关键字哈希表
static HashMap keyword_map;

// 初始化关键字哈希表
static void init_keywords() {
    // CACT 规范中的关键字
    // "true" 和 "false" 被当作字面量处理，而不是关键字
    static char* kw[] = {
        "const", "int", "bool", "float", "double", "void",
        "if", "else", "while", "break", "continue", "return"
    };
    static TokenKind kinds[] = {
        TK_CONST, TK_INT, TK_BOOL, TK_FLOAT, TK_DOUBLE, TK_VOID,
        TK_IF, TK_ELSE, TK_WHILE, TK_BREAK, TK_CONTINUE, TK_RETURN
    };
    
    for (size_t i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        hashmap_put(&keyword_map, kw[i], (void*)(intptr_t)kinds[i]);
    }
}

// 创建一个新的 Token 并追加到链表末尾
static Token* new_token(TokenKind kind, char* start, int len) {
    Token* tok = (Token*)calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = len;
    tok->row = L.row;
    tok->col = L.col - len; // 列号是 token 开始的位置

    if (L.head == NULL) {
        L.head = L.tail = tok;
    } else {
        L.tail->next = tok;
        L.tail = tok;
    }
    return tok;
}

// 错误报告函数
static void lexer_error(const char* msg) {
    fprintf(stderr, "Lexer Error in %s at [%d:%d]: %s\n", L.filename, L.row, L.col, msg);
    exit(1);
}

// 前进一个字符，并更新行列号
static char advance() {
    if (*L.current == '\n') {
        L.row++;
        L.col = 1;
    } else {
        L.col++;
    }
    return *L.current++;
}

// 查看下一个字符但不前进
static char peek() {
    return *L.current;
}

// 查看下下个字符
static char peek_next() {
    if (*L.current == '\0' || *(L.current + 1) == '\0') return '\0';
    return *(L.current + 1);
}

// 处理数字
static Token* number() {
    char* start = L.current;
    
    // 检查十六进制
    if (*start == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume '0'
        advance(); // consume 'x' or 'X'
        while (isxdigit(peek())) advance();
        Token* tok = new_token(TK_NUM_INT, start, L.current - start);
        tok->val.i_val = (int)strtol(start, NULL, 0);
        return tok;
    }

    // 检查八进制和浮点数
    bool is_float = false;
    while (isdigit(peek())) {
        advance();
    }
    if (peek() == '.' && isdigit(peek_next())) {
        is_float = true;
        advance(); // consume '.'
        while(isdigit(peek())) advance();
    }
    if (peek() == 'e' || peek() == 'E') {
        is_float = true;
        advance(); // consume 'e' or 'E'
        if (peek() == '+' || peek() == '-') advance();
        if (!isdigit(peek())) lexer_error("Exponent has no digits.");
        while(isdigit(peek())) advance();
    }

    if (is_float) {
        if (peek() == 'f' || peek() == 'F') {
            advance();
            Token *tok = new_token(TK_NUM_FLOAT, start, L.current - start);
            tok->val.f_val = strtof(start, NULL);
            return tok;
        } else {
            Token *tok = new_token(TK_NUM_DOUBLE, start, L.current - start);
            tok->val.d_val = strtod(start, NULL);
            return tok;
        }
    }

    // 处理十进制和八进制整数
    Token* tok = new_token(TK_NUM_INT, start, L.current - start);
    tok->val.i_val = (int)strtol(start, NULL, 0);
    return tok;
}


// 处理标识符和关键字
static Token* identifier() {
    char* start = L.current;
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }
    
    int len = L.current - start;
    TokenKind kind = (TokenKind)(intptr_t)hashmap_get2(&keyword_map, start, len);

    if (kind) { // 是关键字
        return new_token(kind, start, len);
    }
    
    // 检查布尔字面量
    if (len == 4 && memcmp(start, "true", 4) == 0) {
        Token* tok = new_token(TK_LIT_BOOL, start, len);
        tok->val.b_val = true;
        return tok;
    }
    if (len == 5 && memcmp(start, "false", 5) == 0) {
        Token* tok = new_token(TK_LIT_BOOL, start, len);
        tok->val.b_val = false;
        return tok;
    }

    return new_token(TK_IDENT, start, len); // 是普通标识符
}

// 主函数
Token* tokenize(str source_code, const char* filename) {
    // 初始化Lexer状态
    L.source = source_code;
    L.filename = filename;
    L.current = L.source.body;
    L.row = 1;
    L.col = 1;
    L.head = L.tail = NULL;

    init_keywords();
    
    char* end = L.source.body + L.source.len;
    while (L.current < end) {
        char *start = L.current;
        char c = advance();

        switch(c) {
            // 跳过空白字符
            case ' ': case '\r': case '\t': case '\n': break;
            
            // 注释
            case '/':
                if (peek() == '/') {
                    while (peek() != '\n' && L.current < end) advance();
                } else if (peek() == '*') {
                    advance(); // consume '*'
                    while(L.current < end) {
                        if(peek() == '*' && peek_next() == '/') break;
                        advance();
                    }
                    if (L.current >= end) lexer_error("Unterminated block comment.");
                    advance(); // consume '*'
                    advance(); // consume '/'
                } else {
                    new_token(TK_SLASH, start, 1);
                }
                break;

            // 符号
            case '+': new_token(TK_PLUS, start, 1); break;
            case '-': new_token(TK_MINUS, start, 1); break;
            case '*': new_token(TK_STAR, start, 1); break;
            case '%': new_token(TK_PERCENT, start, 1); break;
            case '(': new_token(TK_L_PAREN, start, 1); break;
            case ')': new_token(TK_R_PAREN, start, 1); break;
            case '{': new_token(TK_L_BRACE, start, 1); break;
            case '}': new_token(TK_R_BRACE, start, 1); break;
            case '[': new_token(TK_L_BRACKET, start, 1); break;
            case ']': new_token(TK_R_BRACKET, start, 1); break;
            case ';': new_token(TK_SEMICOLON, start, 1); break;
            case ',': new_token(TK_COMMA, start, 1); break;
            
            // 可能有两个字符的符号
            case '=': new_token(peek() == '=' ? (advance(), TK_EQ) : TK_ASSIGN, start, L.current - start); break;
            case '!': new_token(peek() == '=' ? (advance(), TK_NEQ) : TK_LOG_NOT, start, L.current - start); break;
            case '<': new_token(peek() == '=' ? (advance(), TK_LE) : TK_LT, start, L.current - start); break;
            case '>': new_token(peek() == '=' ? (advance(), TK_GE) : TK_GT, start, L.current - start); break;
            case '&': if (peek() == '&') { advance(); new_token(TK_LOG_AND, start, 2); } else { lexer_error("Unexpected character '&'"); } break;
            case '|': if (peek() == '|') { advance(); new_token(TK_LOG_OR, start, 2); } else { lexer_error("Unexpected character '|'"); } break;
            
            default:
                if (isalpha(c) || c == '_') {
                    L.current--; L.col--; // 回退一个字符，让 identifier 函数来处理
                    identifier();
                } else if (isdigit(c)) {
                    L.current--; L.col--; // 回退
                    number();
                } else {
                    lexer_error("Unexpected character.");
                }
                break;
        }
    }
    
    new_token(TK_EOF, L.current, 0);
    return L.head;
}

void free_tokens(Token* token) {
    Token* current = token;
    while (current != NULL) {
        Token* next = current->next;
        free(current);
        current = next;
    }
}


// token_kind_to_str 的实现
const char* token_kind_to_str(TokenKind kind) {
    static const char* const names[] = {
        "TK_EOF", "TK_IDENT", "TK_NUM_INT", "TK_NUM_FLOAT", "TK_NUM_DOUBLE", "TK_LIT_BOOL",
        "TK_CONST", "TK_INT", "TK_BOOL", "TK_FLOAT", "TK_DOUBLE", "TK_VOID",
        "TK_IF", "TK_ELSE", "TK_WHILE", "TK_BREAK", "TK_CONTINUE", "TK_RETURN",
        "TK_PLUS", "TK_MINUS", "TK_STAR", "TK_SLASH", "TK_PERCENT",
        "TK_EQ", "TK_NEQ", "TK_LT", "TK_LE", "TK_GT", "TK_GE",
        "TK_L_PAREN", "TK_R_PAREN", "TK_L_BRACE", "TK_R_BRACE", "TK_L_BRACKET", "TK_R_BRACKET",
        "TK_ASSIGN", "TK_SEMICOLON", "TK_COMMA", "TK_LOG_AND", "TK_LOG_OR", "TK_LOG_NOT",
    };
    if (kind >= 0 && kind < sizeof(names)/sizeof(names[0])) {
        return names[kind];
    }
    return "UNKNOWN_TOKEN";
}
