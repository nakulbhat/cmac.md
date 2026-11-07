#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include "html_macros.h"

int main(void){
    STRING raw = {0}, out = {0};

    BODY(raw,
        PARA(Some BOLD(bold) text and a LINK("https://google.com", link).)

             PARA(
        CODE(
            RAW(0, #include <stdio.h>)
            RAW(0, )
            RAW(0, int main(int argc, char **argv){)
            RAW(4, printf("Hello, world!");)
            RAW(4, if(argc < 2 && argv) return 1;)
            RAW(0, })
        )
             )

        PARA(Back to prose.)
    );

    puts("---- intermediate ----");
    puts(raw.str);

    render(&out, raw.str);
    puts("---- rendered ----");
    puts(out.str);

#ifdef CODE_GCC
    STRING raw2 = {0}, out2 = {0};

    BODY(raw2,
        PARA(GNU raw string:)
        CODE_GCC(R"(
#include <stdio.h>
int main(void){
    // comment survives
    printf("Hello, world!\n");
}
)")
    );

    puts("---- GCC intermediate ----");
    puts(raw2.str);

    render(&out2, raw2.str);
    puts("---- GCC rendered ----");
    puts(out2.str);

    free(raw2.str);
    free(out2.str);
#endif

    free(raw.str);
    free(out.str);
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
