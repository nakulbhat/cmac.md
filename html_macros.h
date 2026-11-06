// html_chars.h
#ifndef HTML_CHARS_H
#define HTML_CHARS_H

// All macros will have to follow this hierarchy
// L0 macro: _STRINGIZE
// L1 macro: BODY
// L2 macro (at least n-ary): PARA, LINK, BOLD, SQQ...
// L3 macro (niladic): SQ, DQ, LT...

// Higher level macros may be included in lower level macros, but not vice
// versa. Work in progress, so more macros/levels may be added

// L0 Macros
#define _STRINGIZE(...) #__VA_ARGS__


//
// L1 Macros
//
#define BODY(html_string, ...)                                 \
    append_string(&html_string,                                \
                  "<body>" _STRINGIZE(__VA_ARGS__) "</body>") 


//
// L2 Macros
//
#define PARA(...) <p>__VA_ARGS__</p>
#define BOLD(...) <b>__VA_ARGS__</b>

// LINK should be only used as LINK("https://google.com", google).
// Without the string literal, C won't allow // and html won't work with &sol;
#define LINK(src,...) <a href=src>__VA_ARGS__</a>

// Quotes can be written normally, but will not expand any macros inside them.
// These macros can be used instead for macro expansion
#define SQQ(...)  &apos;__VA_ARGS__&apos;
#define DQQ(...)  &quot;__VA_ARGS__&quot;

//
// L3 Macros
//
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


#endif
