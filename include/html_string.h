#ifndef HTML_STRING_H
#define HTML_STRING_H

#include <stddef.h>

typedef struct {
    char *str;
    size_t len, cap;
} STRING;

void append_string(STRING *parent, const char *append);
void append_char(STRING *s, char c);

#endif
