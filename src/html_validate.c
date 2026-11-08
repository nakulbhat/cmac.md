#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "html_validate.h"

enum { 
    C_ROOT       = 1 << 0,
    C_BLOCK      = 1 << 1,
    C_INLINE     = 1 << 2,
    C_CODE       = 1 << 3,
    T_TEXT       = 1 << 4,
    C_VOID       = 1 << 5,
    C_HTML_CHILD = 1 << 6,
    C_HEAD_CHILD = 1 << 7 
};

static const struct { const char *name; unsigned cat, allows; } TAGS[] = {
    // name         category                    may contain
    { "html",       C_ROOT,                     C_HTML_CHILD },
    { "head",       C_HTML_CHILD,               C_HEAD_CHILD | T_TEXT },
    { "body",       C_HTML_CHILD,               C_BLOCK | C_INLINE | T_TEXT },
    
    // Head children
    { "title",      C_HEAD_CHILD,               T_TEXT },
    { "meta",       C_HEAD_CHILD | C_VOID,      0 },
    { "style",      C_HEAD_CHILD,               T_TEXT },  // Or a custom text/code category
    { "link",       C_HEAD_CHILD | C_VOID,      0 },
    { "script",     C_HEAD_CHILD | C_BLOCK,     T_TEXT },  // Can be in head or body
    
    // Structural / Blocks
    { "p",          C_BLOCK,                    C_INLINE | T_TEXT },
    { "h1",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "h2",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "h3",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "h4",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "h5",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "h6",         C_BLOCK,                    C_INLINE | T_TEXT },
    { "ul",         C_BLOCK,                    C_BLOCK | T_TEXT },
    { "ol",         C_BLOCK,                    C_BLOCK | T_TEXT },
    { "li",         C_BLOCK | C_INLINE,         C_BLOCK | C_INLINE | T_TEXT },
    { "div",        C_BLOCK,                    C_BLOCK | C_INLINE | T_TEXT },
    { "blockquote", C_BLOCK,                    C_BLOCK | C_INLINE | T_TEXT },
    { "hr",         C_BLOCK  | C_VOID,          0 },

    // Inlines / Void / Code
    { "span",       C_INLINE,                   C_INLINE | T_TEXT },
    { "img",        C_INLINE | C_VOID,          0 },
    { "pre",        C_BLOCK,                    C_CODE },
    { "code",       C_INLINE | C_CODE,          T_TEXT },
    { "b",          C_INLINE,                   C_INLINE | T_TEXT },
    { "i",          C_INLINE,                   C_INLINE | T_TEXT },
    { "em",         C_INLINE,                   C_INLINE | T_TEXT },
    { "s",          C_INLINE,                   C_INLINE | T_TEXT },
    { "a",          C_INLINE,                   C_INLINE | T_TEXT },
};

#define NTAGS (sizeof TAGS / sizeof TAGS[0])

static int find_tag(const char *s, size_t n){
    for(size_t i = 0; i < NTAGS; i++)
        if(strlen(TAGS[i].name) == n && !strncmp(TAGS[i].name, s, n))
            return (int)i;
    return -1;
}

typedef struct {
    const char *src, *p;
    const char *last;
    int errors, in_a;
} PARSER;

// Reports the first error only: prints from the last tag to the error point
// (plus the rest of the offending tag, if the error is a tag itself).
static void report_error(PARSER *ps, const char *at, const char *fmt, ...){
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

            // Ignore DOCTYPE and comments (<!...)
            if(*p == '!'){
                while(*ps->p && *ps->p != '>') ps->p++;
                if(*ps->p == '>') ps->p++;
                continue;
            }
            
            int closing = (*p == '/');
            if(closing) p++;

            const char *name = p;
            while(isalnum((unsigned char)*p)) p++;
            size_t nl = (size_t)(p - name);
            if(!nl){
                report_error(ps, start, "stray '<' in text (use LT() or escape it)");
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
                report_error(ps, start, "unterminated tag <%.*s", (int)nl, name);
                ps->p = p;
                return;
            }
            ps->p = p + 1;

            // ps->last is updated only after this tag's own checks, so errors
            // are reported relative to the previous tag.
            int t = find_tag(name, nl);
            if(t < 0){
                report_error(ps, start, "unknown tag <%.*s>", (int)nl, name);
                ps->last = start;
                continue;
            }

            if(closing){
                if(t == parent){ ps->last = start; return; }   // our own closer: done
                report_error(ps, start, "unexpected </%s> inside %s", TAGS[t].name, pname);
                ps->last = start;
                continue;
            }

            if(!(TAGS[t].cat & allowed))
                report_error(ps, start, "<%s> is not allowed inside %s", TAGS[t].name, pname);

            ps->last = start;

            // If it's a void tag, do NOT recurse into parse_content looking for a closer!
            if (!(TAGS[t].cat & C_VOID)) {
                int is_a = !strcmp(TAGS[t].name, "a");
                if(is_a && ps->in_a) report_error(ps, start, "<a> nested inside <a>");
                ps->in_a += is_a;
                parse_content(ps, t);           
                ps->in_a -= is_a;
            }
        }
        else if(*start == '&'){
            const char *p = start + 1;
            while(isalnum((unsigned char)*p) || *p == '#') p++;
            if(*p == ';') p++;
            else report_error(ps, start, "bare '&' or unterminated entity (use AMP() or ENT(name))");
            ps->p = p;
        }
        else {
            if(!isspace((unsigned char)*start) && !text_reported && !(allowed & T_TEXT)){
                report_error(ps, start, "text not allowed directly inside %s", pname);
                text_reported = 1;            // one report per run of text
            }
            ps->p++;
        }
    }

    if(parent >= 0) report_error(ps, ps->p, "<%s> never closed", pname);
}

// Returns the number of errors found (0 = valid). Run on RENDERED output.
int validate_html(const char *html){
    PARSER ps = { html, html, NULL, 0, 0 };
    parse_content(&ps, -1);
    return ps.errors;
}
