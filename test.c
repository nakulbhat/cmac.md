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

void grow_string(STRING *string, int cap);
void append_string(STRING *parent, const char *append);
static void append_char(STRING *s, char c);
static void append_escaped_n(STRING *s, const char *in, size_t len);
void render(STRING *out, const char *in);
static void append_file_range(STRING *out, const char *path, long b, long e,
                              char mode);
static char *read_file(const char *path, size_t *n);


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
        PARA(Use CODEINLINE(if(a < b && c)) to compare.)
        CODEFILE("test.c", 1, 10, 'l')
        CODEFILE("test.c", 1, 0, 'l')    // whole file
        CODEFILE("test.c", 1, 64, 'b')   // first 64 bytes

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


static void append_char(STRING *s, char c){
    char b[2] = { c, '\0' };
    append_string(s, b);
}

static void append_escaped_n(STRING *s, const char *in, size_t len){
    for(size_t i = 0; i < len; i++){
        switch(in[i]){
        case '<': append_string(s, "&lt;");  break;
        case '>': append_string(s, "&gt;");  break;
        case '&': append_string(s, "&amp;"); break;
        default:  append_char(s, in[i]);
        }
    }
}

void render(STRING *out, const char *in){
    const char *RB  = "RAW_B_ ",  *RE  = " RAW_E_";
    const char *GB  = "RAW_G_ R\"(", *GE = ")\" RAW_GE_";
    size_t rb = strlen(RB), re = strlen(RE), gb = strlen(GB), ge = strlen(GE);

    while(*in){
        if(!strncmp(in, RB, rb)){
            in += rb;
            char *p;
            long n = strtol(in, &p, 10);
            in = p;
            if(*in == ' ') in++;
            for(long i = 0; i < n; i++) append_char(out, ' ');
            const char *end = strstr(in, "RAW_E_");
            if(!end) ERROR("unterminated RAW");
            size_t len = (size_t)(end - in);
            if(len && end[-1] == ' ') len--;
            append_escaped_n(out, in, len);
            append_char(out, '\n');
            in = end + strlen("RAW_E_");
            if(*in == ' ') in++;
        }
        else if(!strncmp(in, GB, gb)){
            in += gb;
            const char *end = strstr(in, GE);
            if(!end) ERROR("unterminated CODE_GCC");
            size_t len = (size_t)(end - in);
            if(len && in[0] == '\n'){ in++; len--; }
            if(len && in[len-1] == '\n') len--;
            append_escaped_n(out, in, len);
            append_char(out, '\n');
            in = end + ge;
        }
        else if(!strncmp(in, "RAW_I_ ", 7)){
            in += 7;
            const char *end = strstr(in, " RAW_IE_");
            if(!end) ERROR("unterminated CODEI");
            append_escaped_n(out, in, (size_t)(end - in));
            in = end + strlen(" RAW_IE_");
        }
        else if(!strncmp(in, "RAW_F_ ", 7)){
            in += 7;
            if(*in != '"') ERROR("CODEFILE: filename must be a string literal");
            in++;
            const char *q = strchr(in, '"');
            if(!q) ERROR("CODEFILE: unterminated filename");
            char path[4096];
            size_t pl = (size_t)(q - in);
            if(pl >= sizeof path) ERROR("CODEFILE: path too long");
            memcpy(path, in, pl);
            path[pl] = '\0';
            in = q + 1;

            char *p;
            long b = strtol(in, &p, 10);
            long e = strtol(p, &p, 10);
            while(*p == ' ' || *p == '\'') p++;
            char mode = *p;

            const char *fe = strstr(p, "RAW_FE_");
            if(!fe) ERROR("unterminated CODEFILE");
            append_file_range(out, path, b, e, mode);
            in = fe + strlen("RAW_FE_");
        }
        else {
            append_char(out, *in++);
        }
    }
}

static char *read_file(const char *path, size_t *n){
    FILE *f = fopen(path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if(!buf){ fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    *n = got;
    return buf;
}

// b, e are 1-based inclusive; e <= 0 means EOF
static void append_file_range(STRING *out, const char *path,
                              long b, long e, char mode){
    size_t n;
    char *buf = read_file(path, &n);
    if(!buf) ERROR("CODEFILE: cannot open %s\n", path);
    if(b < 1) b = 1;
    const char *end = buf + n, *from = end, *to = end;

    switch(mode){
    case 'b':
        if((size_t)(b - 1) < n) from = buf + (b - 1);
        if(e > 0 && (size_t)e < n) to = buf + e;
        break;

    case 'l': {
        long line = 1;
        if(b == 1) from = buf;
        for(const char *p = buf; p < end; p++){
            if(*p != '\n') continue;
            if(line == e){ to = p + 1; break; }
            line++;
            if(line == b) from = p + 1;
        }
        break;
    }

    case 'c': {
        long ch = 0;
        for(const char *p = buf; p < end; p++){
            if(((unsigned char)*p & 0xC0) == 0x80) continue;  // UTF-8 continuation
            ch++;
            if(ch == b) from = p;
            if(e > 0 && ch == e + 1){ to = p; break; }
        }
        break;
    }

    default:
        ERROR("CODEFILE: bad mode '%c' (use l, b or c)\n", mode);
    }

    if(to < from) to = from;
    append_escaped_n(out, from, (size_t)(to - from));
    if(to > from && to[-1] != '\n') append_char(out, '\n');
    free(buf);
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
