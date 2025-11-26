#include <stdio.h>
#include "str.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"

// print ast for test
void print_ast(Node *node, int indent) {
    if (!node) return;

    // indent
    for (int i = 0; i < indent; i++) printf("  ");

    switch(node->kind) {
        // Expressions
        case ND_ADD:       printf("+\n"); break;
        case ND_SUB:       printf("-\n"); break;
        case ND_MUL:       printf("*\n"); break;
        case ND_DIV:       printf("/\n"); break;
        case ND_MOD:       printf("%%\n"); break;
        case ND_NEG:       printf("Unary -\n"); break;
        case ND_EQ:        printf("==\n"); break;
        case ND_NE:        printf("!=\n"); break;
        case ND_LT:        printf("<\n"); break;
        case ND_LE:        printf("<=\n"); break;
        case ND_LOG_AND:   printf("&&\n"); break;
        case ND_LOG_OR:    printf("||\n"); break;
        case ND_LOG_NOT:   printf("!\n"); break;
        case ND_ASSIGN:    printf("=\n"); break;
        case ND_VAR:       printf("Var: %s\n", node->var->name); break;
        case ND_NUM_INT:   printf("Int: %d\n", node->i_val); break;
        case ND_NUM_FLOAT: printf("Float: %f\n", node->f_val); break;
        case ND_NUM_DOUBLE:printf("Double: %f\n", node->d_val); break;
        case ND_BOOL:      printf("Bool: %s\n", node->b_val ? "true" : "false"); break;
        case ND_FUNC_CALL: printf("Call: %s\n", node->func_name); break;
        case ND_ARRAY_ACCESS: printf("ArrayAccess\n"); break;

        // Statements
        case ND_RETURN:    printf("Return\n"); break;
        case ND_IF:        printf("If\n"); break;
        case ND_WHILE:     printf("While\n"); break;
        case ND_BLOCK:     printf("Block\n"); break;
        case ND_BREAK:     printf("Break\n"); break;
        case ND_CONTINUE:  printf("Continue\n"); break;
        case ND_EXPR_STMT: printf("ExprStmt\n"); break;
        
        default: printf("Unknown Node Kind: %d\n", node->kind);
    }

    // recursive for child nodes
    print_ast(node->lhs, indent + 1);
    print_ast(node->rhs, indent + 1);
    print_ast(node->cond, indent + 1);
    print_ast(node->then, indent + 1);
    print_ast(node->els, indent + 1);
    print_ast(node->index, indent + 1);

    // for block and funciton
    if (node->kind == ND_BLOCK) {
        for(Node* n = node->body; n; n = n->next) {
            print_ast(n, indent + 1);
        }
    } else if (node->kind == ND_FUNC_CALL) {
        for(Node* n = node->args; n; n = n->next) {
            print_ast(n, indent + 1);
        }
    }

    // print next 
    print_ast(node->next, indent);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <filename.cact>\n", argv[0]);
        return 1;
    }

    // --- 1. read file ---
    str content = str_fread(argv[1]);
    if (content.body == NULL) {
        return 1;
    }

    // --- init the reporting system ---
    //init_error_reporting(content.body, argv[1]);

    // --- 2. lex ---
    printf("--- 1. Lexing ---\n");
    Token *tok = tokenize(content, argv[1]);
    printf("Lexing done.\n\n");

    // --- 3. parse ---
    printf("--- 2. Parsing ---\n");
    Obj *prog = parse(tok);
    printf("Parsing done.\n\n");

    // --- 4. print ---
    printf("--- 3. Program Arch ---\n");
    for (Obj *obj = prog; obj; obj = obj->next) {
        if (!obj->is_function) {
            printf("Global value: %s\n", obj->name);
        } else {
            printf("function: %s\n", obj->name);
            printf("  args:\n");
            if (!obj->params) {
                printf("    (None)\n");
            } else {
                for (Obj* p = obj->params; p; p=p->next) {
                    printf("    - %s\n", p->name);
                }
            }
            printf("  function body:\n");
            print_ast(obj->body, 2);
        }
        printf("\n");
    }

    str_free(content);
    return 0;
}

