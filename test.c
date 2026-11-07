#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "html_macros.h"

#define PATH_MAX 4096
#define ERROR(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE);} while(0)
#define ABORT(...) do { fprintf(stderr, __VA_ARGS__); abort();} while(0)

enum { C_ROOT = 1, C_BLOCK = 2, C_INLINE = 4, C_CODE = 8, T_TEXT = 16 };

static const struct { const char *name; unsigned cat, allows; } TAGS[] = {
    // name    category             may contain
    { "body", C_ROOT,              C_BLOCK | C_INLINE | T_TEXT },
    { "p",    C_BLOCK,             C_INLINE | T_TEXT },
    { "pre",  C_BLOCK,             C_CODE },               // only <code>, no text
    { "code", C_INLINE | C_CODE,   T_TEXT },               // text only
    { "b",    C_INLINE,            C_INLINE | T_TEXT },
    { "i",    C_INLINE,            C_INLINE | T_TEXT },
    { "em",   C_INLINE,            C_INLINE | T_TEXT },
    { "s",    C_INLINE,            C_INLINE | T_TEXT },
    { "a",    C_INLINE,            C_INLINE | T_TEXT },
};

#define NTAGS (sizeof TAGS / sizeof TAGS[0])

static int find_tag(const char *s, size_t n){
    for(size_t i = 0; i < NTAGS; i++)
        if(strlen(TAGS[i].name) == n && !strncmp(TAGS[i].name, s, n))
            return (int)i;
    return -1;
}

typedef struct {
    char *str;
    size_t len, cap;
} STRING;

typedef struct {
    const char *src, *p;
    const char *last;
    int errors, in_a;
} PARSER;

void grow_string(STRING *string, int cap);
void append_string(STRING *parent, const char *append);
static void append_char(STRING *s, char c);
static void append_escaped_html_string(STRING *s, const char *in, size_t len);
void render(STRING *out, const char *in);
static void append_file_range(STRING *out, const char *path, long b, long e,
                              char mode);
static char *read_file(const char *path, size_t *n);
static void verr(PARSER *ps, const char *at, const char *fmt, ...);
static void parse_content(PARSER *ps, int parent);
static int find_tag(const char *s, size_t n);

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

static void append_escaped_html_string(STRING *s, const char *in, size_t len){
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

    // define markers used
    const char *RB  = "RAW_B_ "; // RAW Begin
    const char *RE  = " RAW_E_"; // RAW End
    const char *GB  = "RAW_G_ R\"("; // RAW GCC Begin
    const char *GE = ")\" RAW_GE_";  // RAW GCC End
    const char *IB = "RAW_IB_ ";  // RAW Inline Begin
    const char *IE = " RAW_IE_"; // RAW Inline End
    const char *FB = "RAW_FB_ "; // RAW File Begin
    const char *FE = " RAW_FE_"; // RAW File End
    size_t rb = strlen(RB), re = strlen(RE), gb = strlen(GB), ge = strlen(GE);
    size_t ib = strlen(IB), ie = strlen(IE), fb = strlen(FB), fe = strlen(FE);

    while(*in){
        if(!strncmp(in, RB, rb)){ // Regular RAW macro

            // We have to process "RAW_B_ 4 This is indented 4 spaces RAW_E_"
            // Some cases have RAW(0, ) which gives "RAW_B_ 0 RAW_E_"

            in += rb; // jump forward by marker width

            // Extract indent level and skip th number itself
            char *p;
            long num_indent = strtol(in, &p, 10);
            in = p;
            if(*in == ' ') in++; // there will be a space after num_indent, skip it
            for(long i = 0; i < num_indent; i++) append_char(out, ' ');

            // Find the end of the RAW macro. `in` has already skipped the prefix space
            const char *end = strstr(in, "RAW_E_");
            if(!end) ERROR("unterminated RAW");

            // Append that string into out
            // in some cases, like an emtpy line "RAW_B_ 0 RAW_E_"
            // the sentence itself is zero width and the sentence append call is nop

            size_t len = (size_t)(end - in);
            if(len && end[-1] == ' ') len--; // defensively remove space before "RAW_E_"
            append_escaped_html_string(out, in, len);

            append_char(out, '\n');
            in = end + strlen("RAW_E_");
            if(*in == ' ') in++; // Skip the extra joining space so the next loop begins with "RAW_B...
        }
        else if(!strncmp(in, GB, gb)){ // GCC Raw macro
            // We have to process RAW_G_ R"(multiline \n code)" RAW_GE_
            // essentially the same process as above, but does not care for indents, as
            // raw strings are written to string literals 

            in += gb;
            const char *end = strstr(in, GE);
            if(!end) ERROR("unterminated CODE_GCC");
            size_t len = (size_t)(end - in);
            if(len && in[0] == '\n'){ in++; len--; }
            if(len && in[len-1] == '\n') len--;
            append_escaped_html_string(out, in, len);
            append_char(out, '\n');
            in = end + ge;
        }
        else if(!strncmp(in, IB, ib)){ // Inline RAW
            // Not much to do here, just check for termination
            // and escape chars.
            in += ib;
            const char *end = strstr(in, " RAW_IE_");
            if(!end) ERROR("unterminated CODEINLINE");
            append_escaped_html_string(out, in, (size_t)(end - in));
            in = end + strlen(" RAW_IE_");
        }
        else if(!strncmp(in, FB, fb)){ // File RAW
            // filename has to be a string literal, otherwise it may be
            // split up by the compiler.
            in += fb;
            if(*in != '"') ERROR("CODEFILE: filename must be a string literal");

            in++; // skip the quote itself 

            // find the end of the filename
            const char *q = strchr(in, '"');
            if(!q) ERROR("CODEFILE: unterminated filename");

            char path[PATH_MAX];
            size_t pl = (size_t)(q - in);
            if(pl >= sizeof path) ERROR("CODEFILE: path too long");
            memcpy(path, in, pl);
            path[pl] = '\0';

            // get the begin and end lines/bytes/chars
            in = q + 1;
            char *p;
            long b = strtol(in, &p, 10);
            long e = strtol(p, &p, 10);
            
            // get mode 'l'/'b'/'c'
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

// b, e are 0-based inclusive; e < 0 means EOF
static void append_file_range(STRING *out, const char *path,
                            long b, long e, char mode){
    size_t n;
    char *buf = read_file(path, &n);
    if(!buf) ERROR("CODEFILE: cannot open %s\n", path);
    if(b < 0) b = 0;
    const char *end = buf + n, *from = end, *to = end;

    switch(mode){
    case 'b': // bytes
        if((size_t)b < n) from = buf + b;
        if(e >= 0) {
            if((size_t)(e + 1) < n) to = buf + e + 1;
            else to = end;
        }
        break;

    case 'l': { // lines
        long line = 0;
        if(b == 0) from = buf;
        for(const char *p = buf; p < end; p++){
            if(*p != '\n') continue;
            if(line == e){ to = p + 1; break; }
            line++;
            if(line == b) from = p + 1;
        }
        break;
    }

    case 'c': { // chars
        long ch = 0;
        for(const char *p = buf; p < end; p++){
            if(((unsigned char)*p & 0xC0) == 0x80) continue;  // UTF-8 continuation
            if(ch == b) from = p;
            if(e >= 0 && ch == e + 1){ to = p; break; }
            ch++;
        }
        break;
    }

    default:
        ERROR("CODEFILE: bad mode '%c' (use l, b or c)\n", mode);
    }

    if(to < from) to = from;
    append_escaped_html_string(out, from, (size_t)(to - from));
    if(to > from && to[-1] != '\n') append_char(out, '\n');
    free(buf);
}

// Reports the first error only: prints from the last tag to the error point
// (plus the rest of the offending tag, if the error is a tag itself).
static void verr(PARSER *ps, const char *at, const char *fmt, ...){
    if(ps->errors++) return;                       // stop after the first error

    fprintf(stderr, "html error: ");
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);

    const char *from = ps->last ? ps->last : ps->src;
    if(from > at) from = at;
    const char *to = at;
    if(*to == '<'){                                 // include the offending tag
        while(*to && *to != '>') to++;
        if(*to) to++;
    }
    fprintf(stderr, "  context: %.*s\n", (int)(to - from), from);
    fprintf(stderr, "  %*s^\n", 11 + (int)(at - from), "");   // caret under the error
}

// Parses the content of one element (parent = index into TAGS, or -1 for top
// level) and returns after consuming that element's closing tag.
static void parse_content(PARSER *ps, int parent){
    unsigned allowed = parent >= 0 ? TAGS[parent].allows : C_ROOT;
    const char *pname = parent >= 0 ? TAGS[parent].name : "top level";
    int text_reported = 0;

    while(*ps->p){
        const char *start = ps->p;

        if(*start == '<'){
            text_reported = 0;
            const char *p = start + 1;
            int closing = (*p == '/');
            if(closing) p++;

            const char *name = p;
            while(isalnum((unsigned char)*p)) p++;
            size_t nl = (size_t)(p - name);
            if(!nl){
                verr(ps, start, "stray '<' in text (use LT() or escape it)");
                ps->p = start + 1;
                continue;
            }

            char q = 0;                       // skip attributes, respecting quotes
            while(*p && (q || *p != '>')){
                if(q){ if(*p == q) q = 0; }
                else if(*p == '"' || *p == '\'') q = *p;
                p++;
            }
            if(*p != '>'){
                verr(ps, start, "unterminated tag <%.*s", (int)nl, name);
                ps->p = p;
                return;
            }
            ps->p = p + 1;

            int t = find_tag(name, nl);
            if(t < 0){ verr(ps, start, "unknown tag <%.*s>", (int)nl, name); continue; }

            if(closing){
                if(t == parent) return;       // our own closer: done
                verr(ps, start, "unexpected </%s> inside %s", TAGS[t].name, pname);
                continue;
            }

            if(!(TAGS[t].cat & allowed))
                verr(ps, start, "<%s> is not allowed inside %s", TAGS[t].name, pname);

            int is_a = !strcmp(TAGS[t].name, "a");
            if(is_a && ps->in_a) verr(ps, start, "<a> nested inside <a>");
            ps->in_a += is_a;
            parse_content(ps, t);             // recurse into the child
            ps->in_a -= is_a;
        }
        else if(*start == '&'){
            const char *p = start + 1;
            while(isalnum((unsigned char)*p) || *p == '#') p++;
            if(*p == ';') p++;
            else verr(ps, start, "bare '&' or unterminated entity (use AMP() or ENT(name))");
            ps->p = p;
        }
        else {
            if(!isspace((unsigned char)*start) && !text_reported && !(allowed & T_TEXT)){
                verr(ps, start, "text not allowed directly inside %s", pname);
                text_reported = 1;            // one report per run of text
            }
            ps->p++;
        }
    }

    if(parent >= 0) verr(ps, ps->p, "<%s> never closed", pname);
}

// Returns the number of errors found (0 = valid). Run on RENDERED output.
int validate_html(const char *html){
    PARSER ps = { html, html, 0, 0 };
    parse_content(&ps, -1);
    return ps.errors;
}

// Each case: rendered HTML and how many errors validate_html should report.
static const struct { const char *name, *html; int expect; } CASES[] = {
    { "valid document",
      "<body><p>Hi <b>bold</b> &amp; <a href=\"x\">link</a></p> "
      "<pre><code>a &lt;b</code></pre></body>", 0 },

    { "pre inside p (PARA(CODE(...)))",
      "<body><p><pre><code>x</code></pre></p></body>", 1 },

    { "p inside p",
      "<body><p><p>x</p></p></body>", 1 },

    { "a inside a",
      "<body><p><a>x<a>y</a></a></p></body>", 1 },

    { "text directly in pre",
      "<body><pre>text</pre></body>", 1 },

    { "b inside pre",
      "<body><pre><b>x</b></pre></body>", 1 },

    { "stray < in prose",
      "<body><p>a < b</p></body>", 1 },

    { "bare & in prose",
      "<body><p>a & b</p></body>", 1 },

    { "unknown tag",
      "<body><p><span>x</span></p></body>", 2 },   // open and close both flagged

    { "mismatched closer",
      "<body><p>x</b></p></body>", 1 },

    { "unclosed p",
      "<body><p>x</body>", 3 },   // unexpected </body>, p never closed, body never closed

    { "text outside body",
      "hello", 1 },
};

int main(void){
    int failed = 0;
    size_t ncases = sizeof CASES / sizeof CASES[0];

    // ---- Part 1: direct tests on rendered HTML ----
    for(size_t i = 0; i < ncases; i++){
        printf("[%s]\n", CASES[i].name);
        int got = validate_html(CASES[i].html);
        if(got == CASES[i].expect){
            printf("  PASS (%d errors)\n", got);
        } else {
            printf("  FAIL: expected %d errors, got %d\n", CASES[i].expect, got);
            failed++;
        }
    }

    // ---- Part 2: through the real macro pipeline ----
    {
        STRING raw = {0}, out = {0};
        BODY(raw,
            PARA(Some BOLD(bold) text and CODEINLINE(a < b).)
            CODE(
                RAW(0, int main(void){)
                RAW(4, return 1 < 2 && 3 > 2;)
                RAW(0, })
            )
        );
        render(&out, raw.str);
        printf("[pipeline: valid macros]\n");
        int got = validate_html(out.str);
        if(got == 0) printf("  PASS\n"); else { printf("  FAIL: %d errors\n", got); failed++; }
        free(raw.str); free(out.str);
    }
    {
        STRING raw = {0}, out = {0};
        BODY(raw,
            PARA(
                CODE(
                    RAW(0, int x;)
                )
            )
        );
        render(&out, raw.str);
        printf("[pipeline: PARA(CODE(...)) must be rejected]\n");
        int got = validate_html(out.str);
        if(got == 1) printf("  PASS\n"); else { printf("  FAIL: expected 1, got %d\n", got); failed++; }
        free(raw.str); free(out.str);
    }

    printf("\n%s (%d failed)\n", failed ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failed);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
