// html_chars.h
#ifndef HTML_CHARS_H
#define HTML_CHARS_H

// All macros will have to follow this hierarchy
//
//     L3 macro (primitive): _STRINGIZE
//     L2 macro (container): BODY
//     L1 macro (n-ary elements): PARA, LINK, BOLD, SQQ...
//     L0 macro (niladic entities): SQ, DQ, LT...
//
// Lower level macros may be included in higher level macros, but not the other
// way around. This will be enforced by a validate_html function.

/********************************L3 Macros*************************************/
#define _STRINGIZE(...) #__VA_ARGS__


/********************************L2 Macros*************************************/
// BODY will be the highest level macro that can be written in function (page)
// bodies
#define BODY(html_string, ...)                                 \
    append_string(&html_string,                                \
                  "<body>" _STRINGIZE(__VA_ARGS__) "</body>") 


/********************************L1 Macros*************************************/
#define PARA(...) <p>__VA_ARGS__</p>
#define BOLD(...) <b>__VA_ARGS__</b>
#define ITAL(...) <i>__VA_ARGS__</i>
#define EMPH(...) <em>__VA_ARGS__</em>
#define STRK(...) <s>__VA_ARGS__</s>

// Code block should be written as follows:
//     CODE(
//         RAW(0, #include <stdio.h>)
//         RAW()
//         RAW(0, int main(int argc, char **argv){)
//         RAW(4,     printf("Hello, world!"))
//         RAW(0, })
//     )
#define CODE(...)    <pre><code>__VA_ARGS__</code></pre>
#define RAW(n, ...)  RAW_B_ n __VA_ARGS__ RAW_E_

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)

// When compiled on gcc with -std=gnu99/gnu11/gnu17, a shortcut macro can be
// used as follows
#define CODE_GCC(...)  <pre><code>RAW_G_ __VA_ARGS__ RAW_GE_</code></pre>

#endif

// LINK should be only used as LINK("https://google.com", google).
// Without the string literal, C won't allow // and html won't work with &sol;
#define LINK(src,...) <a href=src>__VA_ARGS__</a>

// Quotes can be written normally, but will not expand any macros inside them.
// These macros can be used instead for macro expansion
#define SQQ(...)  &apos;__VA_ARGS__&apos;
#define DQQ(...)  &quot;__VA_ARGS__&quot;

/********************************L0 Macros*************************************/
#define SQ()      &apos;
#define DQ()      &quot;
#define LP()      &lpar;
#define RP()      &rpar;
#define BSLASH()  &bsol;
#define SLASH()   &sol;     
#define DSLASH()  &sol;&sol;
#define LT()      &lt;
#define AMP()     &amp;
#define NBSP()    &nbsp;
#define ENT(name) &name;

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
        else {
            append_char(out, *in++);
        }
    }
}

#endif
