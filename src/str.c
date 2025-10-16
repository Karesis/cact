#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "str.h"

str str_fromcs(const char* cstr)
{
    return (str){(char*)cstr, strlen(cstr)};
}

void str_print(str s)
{
    printf("%.*s\n", (int)s.len, s.body);
}

void _str_dbg(str s, const char* s_name, const char* file, int line) {
    // 格式化输出，包含所有调试信息
    printf("[Debug] %s:%d: %s\n", file, line, s_name);
    printf("{\n");
    printf("    body:\n\"%.*s\"\n", (int)s.len, s.body);
    printf("    len: %zu\n", s.len);
    printf("}\n");
}

str str_fread(const char* fname)
{
    FILE* fp = fopen(fname, "rb");
    if (fp == NULL)
    {
        perror("(str_fread)Cannot open file");
        return STR_FREAD_FAILED;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = (size_t)ftell(fp);
    if (size == -1)
    {
        perror("(str_fread)ftell failed");
        return STR_FREAD_FAILED;
    }
    rewind(fp);
    char* buffer = (char*)malloc(size);
    if (buffer == NULL)
    {
        fprintf(stderr, "(str_fread)Failed to allocate memory for file content.");
        fclose(fp);
        return STR_FREAD_FAILED;
    }
    size_t b_read = fread(buffer, 1, size, fp);
    if (b_read < size) 
    {
        if (feof(fp)) 
            fprintf(stderr, "(str_fread)Unexpected end of file while reading.\n");
        else if (ferror(fp)) 
            perror("(str_fread)Fread to buffer failed");
        fclose(fp);
        free(buffer);
        return STR_FREAD_FAILED;
    }
    if (fclose(fp))
    {
        perror("(str_fread)Cannot close file.");
        return STR_FREAD_FAILED;
    }
    return (str){buffer, size};
}

bool str_cmp(str s, str o)
{
    if (s.len != o.len)
    {
        return false;
    }
    if (s.len == 0) 
    { 
        return true;
    }
    return memcmp(s.body, o.body, s.len) == 0;
}

void str_free(str s) {
    if (s.body != NULL) {
        free(s.body);
    }
}