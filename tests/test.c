#include <stdio.h>
#include <stdlib.h>

#include "html_macros.h"
#include "html_string.h"
#include "html_render.h"
#include "html_validate.h"

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
