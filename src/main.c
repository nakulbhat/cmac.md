#include <stdio.h>
#include <stdlib.h>

#include "html_macros.h"
#include "html_string.h"
#include "html_render.h"
#include "html_validate.h"
#include "assets.h"

int main(void){
    STRING raw = {0}, out = {0};

    BODY(raw,
        PARA( ID(golden) Hello, BOLD(world) and CODEINLINE(a < b).)
        CODE( ID(golden) 
            RAW(0, #include <stdio.h>)
            RAW(0, )
            RAW(0, int main(void){)
            RAW(4, puts("hi");)
            RAW(0, })
        )
    );

    static const unsigned char pdf[] = {
         #embed "test.pdf"
         };
    
    ASSET("out/assets/a.pdf", pdf);

    render(&out, raw.str);
    puts(out.str);

    int errors = validate_html(out.str);
    puts(errors ? "invalid html" : "valid html");

    free(raw.str);
    free(out.str);
    return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
