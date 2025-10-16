#include <stdio.h>
#include "str.h"
#include "lexer.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    str source = str_fread(filename);

    if (source.body == NULL) {
        // str_fread already printed an error
        return 1;
    }

    Token* tokens = tokenize(source, filename);

    printf("--- Tokens for %s ---\n", filename);
    for (Token* tok = tokens; tok; tok = tok->next) {
        printf("[%03d:%03d] %-15s '%.*s'\n", 
               tok->row, 
               tok->col, 
               token_kind_to_str(tok->kind), 
               tok->len, 
               tok->loc);
    }
    printf("--- End of Tokens ---\n");

    // 清理内存
    free_tokens(tokens);
    str_free(source);

    return 0;
}
