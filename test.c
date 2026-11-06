#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST(x) #x

// Found at
// https://stackoverflow.com/questions/2751870/how-exactly-does-the-double-stringize-trick-work
#define _PARA(...) #__VA_ARGS__ // Macro to do the stringize
// #define PARA(...) _PARA(<p>__VA_ARGS__</p>) // Make the preprocessor expand all args first
#define BOLD(...) <b>__VA_ARGS__</b>

#define PARA(...) append_string(&html_string, _PARA(<p>__VA_ARGS__</p>)) 

#define ERROR(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE);} while(0)
#define ABORT(...) do { fprintf(stderr, __VA_ARGS__); abort();} while(0)

typedef struct {
    char *str;
    size_t len, cap;
} STRING;

void grow_string(STRING *string, int cap){
    if(!string) ERROR("grow_string() called without string");
    char *p = realloc(string->str, cap + string->cap);
    if (!p) ABORT("realloc failed");
    string->str = p;
    string->cap += cap;
}

void append_string(STRING *parent, const char *append){
    if(!parent || !append) ERROR("string_append() called with incorrect args");


    char *string = parent->str;
    size_t alen = strlen(append);

    if(parent->len + alen + 1 > parent->cap){
        grow_string(parent, parent->len + alen + 1 - parent->cap);
    }

    memcpy(parent->str + parent->len, append, alen);
    parent->len = parent->len + alen;
    parent->str[parent->len] = '\0';

}




int main(int argc, char **argv){
    STRING html_string = {0};

    PARA(This is some text.);

    PARA(This is some more text, with a comma and a BOLD BOLD(word));

    puts(html_string.str);
    free(html_string.str);
    return 0;
}
