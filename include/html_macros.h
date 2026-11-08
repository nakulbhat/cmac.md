#ifndef HTML_MACROS_H
#define HTML_MACROS_H

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
// HEAD_SECTION and BODY_SECTION will be the highest level macro that can be
// written in function (page) bodies
#define HEAD_SECTION(html_string, ...) \
    append_string(&html_string, "<!DOCTYPE html><html><head>" _STRINGIZE(__VA_ARGS__) "</head>")

#define BODY_SECTION(html_string, ...) \
    append_string(&html_string, "<body>" _STRINGIZE(__VA_ARGS__) "</body></html>")

#define HEAD(...) <head>__VA_ARGS__</head>
#define TITLE(...) <title>__VA_ARGS__</title>
#define META(attr) <meta attr>

/********************************L1 Macros*************************************/
#define PARA(...) <p>__VA_ARGS__</p>
#define BOLD(...) <b>__VA_ARGS__</b>
#define ITAL(...) <i>__VA_ARGS__</i>
#define EMPH(...) <em>__VA_ARGS__</em>
#define STRK(...) <s>__VA_ARGS__</s>

// Code block should be written as follows:
//     CODE(
//         RAW(0, #include <stdio.h>)
//         RAW(0, )
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

#define CODEINLINE(...)  <code>RAW_IB_ __VA_ARGS__ RAW_IE_</code>

// CODEFILE("path", begin, end, 'l')   mode: 'l'ines, 'b'ytes, 'c'hars (UTF-8)
#define CODEFILE(file, b, e, m)  <pre><code>RAW_FB_ file b e m RAW_FE_</code></pre>

// LINK should be only used as LINK("https://google.com", google).
// Without the string literal, C won't allow // and html won't work with &sol;
#define LINK(src,...) <a href=src>__VA_ARGS__</a>

// Quotes can be written normally, but will not expand any macros inside them.
// These macros can be used instead for macro expansion
#define SQQ(...)  &apos;__VA_ARGS__&apos;
#define DQQ(...)  &quot;__VA_ARGS__&quot;
//
// Attributes must come first inside the element they belong to:
//     PARA(ID(golden) CLASS(intro) text here)  ->  <p id="golden" class="intro">text here</p>
// ATTR(name, "value") takes a literal name and a quoted string value.
// ATTR_BOOL(name) emits a valueless attribute such as `hidden` or `disabled`.
#define ATTR(name, ...)  ATTR_B_ name=__VA_ARGS__ ATTR_E_
#define ATTR_BOOL(name)  ATTR_B_ name ATTR_E_

// Shortcuts: the argument is stringized, so write ID(golden), not ID("golden")
#define ID(x)     ATTR(id, #x)
#define CLASS(x)  ATTR(class, #x)

// Headings
#define H1(...) <h1>__VA_ARGS__</h1>
#define H2(...) <h2>__VA_ARGS__</h2>
#define H3(...) <h3>__VA_ARGS__</h3>
#define H4(...) <h4>__VA_ARGS__</h4>
#define H5(...) <h5>__VA_ARGS__</h5>
#define H6(...) <h6>__VA_ARGS__</h6>

// Lists
#define UL(...) <ul>__VA_ARGS__</ul>
#define OL(...) <ol>__VA_ARGS__</ol>
#define LI(...) <li>__VA_ARGS__</li>

// Generic Containers
#define DIV(...)  <div>__VA_ARGS__</div>
#define SPAN(...) <span>__VA_ARGS__</span>

// Blockquotes & Layout
#define BLOCKQUOTE(...) <blockquote>__VA_ARGS__</blockquote>
#define HR() <hr>

// IMG("path", "alt text", CLASS(x) ...)   (attributes, if any, come last)
// Void element: expands to <img> followed by attributes, which render splices
// into the tag. Both src and alt must be string literals. For a purely
// decorative image, pass "" as the alt text.
#define IMG(src_, alt_, ...)  <img> ATTR(src, src_) ATTR(alt, alt_) __VA_ARGS__

/********************************CSS & JS Macros*************************************/
// Inline CSS style block (uses RAW to prevent HTML escaping inside the stylesheet)
#define STYLE(...)          <style>__VA_ARGS__</style>

// External CSS stylesheet link
#define LINK_CSS(href_path)   <link rel="stylesheet" href=href_path>

// Inline JavaScript block
#define SCRIPT_INLINE(...)  <script>__VA_ARGS__</script>

// External JavaScript source link
#define SCRIPT_SRC(src_path)  <script src=src_path></script>

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

// Unbalanced parens inside RAW/CODE: expand to an inert marker that render() converts
#define LPAR  RAW_LP_
#define RPAR  RAW_RP_

#endif
