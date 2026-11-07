#include <string.h>

#include "html_string.h"
#include "internal.h"

// Ensures string->cap >= min_cap, doubling so appends are amortised O(1).
static void string_grow(STRING *string, size_t min_cap){
    if(!string) ERROR("string_grow() called without string\n");
    size_t cap = string->cap ? string->cap : 16;
    while(cap < min_cap) cap *= 2;
    char *p = realloc(string->str, cap);
    if (!p) ABORT("realloc failed");
    string->str = p;
    string->cap = cap;
}

void append_string(STRING *parent, const char *append){
    if(!parent || !append) ERROR("append_string() called with incorrect args\n");

    size_t alen = strlen(append);

    if(parent->len + alen + 1 > parent->cap){
        string_grow(parent, parent->len + alen + 1);
    }

    memcpy(parent->str + parent->len, append, alen);
    parent->len = parent->len + alen;
    parent->str[parent->len] = '\0';

}


void append_char(STRING *s, char c){
    char b[2] = { c, '\0' };
    append_string(s, b);
}
