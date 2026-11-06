#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html_macros.h"

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

    BODY( html_string,
    PARA(This is some text.);

    PARA(This is some more text, with a comma and a BOLD BOLD(word));

    PARA(This is a para with a BOLD(LINK("https://google.com", google link)));
    PARA(LINK("/colophon", BOLD(colophon)));

    PARA(Call foo PAREN(x, y) then index arr`BRACKET(0) and use BRACE(1, 2, 3).);
    PARA(Type ANGLE(div) to open a block, or use a SLASH()SLASH() comment.);
    PARA(Use CDQQ(smart quotes) and don&rsquo;t forget the AMP() sign.);

    PARA(Divide a SLASH() b, escape with BSLASH()n, comment out with SLASH()STAR() ... STAR()SLASH().);
    );
    puts(html_string.str);
    free(html_string.str);
    return 0;
}

// ideal fn
//
//void index(){
// 	STRING html_string = {0}
// 	PROLOGUE(html_string, nakulbhat/)
// 	BODY(html_string,
// 		PARA(...);
// 		PARA(...);
// 	);
// 	EPILOGUE(html_string);
// 	write_string(filename, html_string);
// }
