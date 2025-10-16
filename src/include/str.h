#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    char* body;
    size_t len;
}
str;

#define STR_FREAD_FAILED (str){NULL, 0}

// creat str from a c-style string
str str_fromcs(const char* cstr);

// print a str (just body and a \n)
void str_print(str s);

// print a str for debug
void _str_dbg(str s, const char* s_name, const char* file, int line);

#define str_dbg(s) \
    do { \
        _str_dbg(s, #s, __FILE__, __LINE__); \
    } while(0)

// read str from a file
str str_fread(const char* fname);

// compare two string
bool str_cmp(str s, str o);

void str_free(str s);

#endif 