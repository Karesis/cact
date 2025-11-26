#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "token.h"

static char *current_input;
static char *current_filename;

// report a bug and exit
void error(char *fmt, ...) 
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// Takes a printf-style format string and returns a formatted string.
char *format(char *fmt, ...) 
{
  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fclose(out);
  return buf;
}

void error_tok(Token *tok, char *fmt, ...) 
{
    // print filename and colume
    fprintf(stderr, "%s:%d:%d: ", current_filename, tok->row, tok->col);

    // print format error message
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    // --- print error code and some notices ---

    // find the start for the line
    char *line_start = tok->loc;
    while (current_input < line_start && line_start[-1] != '\n') {
        line_start--;
    }

    // find the end
    char *line_end = tok->loc;
    while (*line_end != '\n' && *line_end != '\0') {
        line_end++;
    }

    // print
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);

    // print the signed symbol (^)
    int indent = tok->loc - line_start;
    fprintf(stderr, "%*s", indent, ""); // print pre-blank
    fprintf(stderr, "^\n");

    exit(1);
}