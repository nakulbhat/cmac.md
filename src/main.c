#include <stdio.h>
#include <stdlib.h>

#include "html_macros.h"
#include "html_string.h"
#include "html_render.h"
#include "html_validate.h"
#include "assets.h"

int main(void){
    STRING raw = {0}, out = {0};

    // Create binary/text assets using byte arrays
    static const unsigned char svg_asset[] = 
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"50\" height=\"50\">"
        "<rect width=\"50\" height=\"50\" fill=\"#111111\"/></svg>";
    ASSET_STR("out/assets/black.svg", svg_asset);

    static const unsigned char js_asset[] = 
        "console.log('cmacmd script executing successfully.');";
    ASSET_STR("out/assets/script.js", js_asset);

    HEAD_SECTION(raw,
        TITLE(Complete cmacmd Test Suite)
        META(charset="utf-8")
        LINK_CSS("out/assets/style.css")
        STYLE(
            RAW(0, body { background-color: purple; color: #222; font-family: system-ui, sans-serif; margin: 40px; })
            RAW(0, h1 { border-bottom: 2px solid #ddd; padding-bottom: 6px; })
            RAW(0, blockquote { border-left: 4px solid #0066cc; margin-left: 0; padding-left: 16px; color: #555; })
        )
        SCRIPT_SRC("out/assets/script.js")
        SCRIPT_INLINE(
            RAW(0, window.addEventListener LPAR 'DOMContentLoaded', () => {)
            RAW(4, console.log('Inline script active.');)
            RAW(0, } RPAR )
        )
    );

    BODY_SECTION(raw,
        H1(ID(top) Complete Tag AMP() Asset Test Suite)
        
        H2(Headings Hierarchy)
        H3(Heading Level 3)
        H4(Heading Level 4)
        H5(Heading Level 5)
        H6(Heading Level 6)

        H2(Text Formatting AMP() Entities)
        PARA(
            This text tests BOLD(bold formatting), ITAL(italic styling), 
            EMPH(emphasis), and STRK(strikethrough). 
            It also includes inline code CODEINLINE(size_t len = 0;), a hyperlink LINK("https://example.com", External Link), 
            and quotes using SQQ(single quotes) and DQQ(double quotes).
        )

        HR()

        H2(Containers AMP() Attributes)
        DIV(
            CLASS(content-box)
            ATTR(data-testid, "container")
            PARA(This paragraph lives inside a custom DIV container.)
            SPAN(Inline text span element here.)
        )

        H2(Blockquotes)
        BLOCKQUOTE(
            Any fool can write code that a computer can understand. Good programmers write code that humans can understand. — Martin Fowler
        )

        H2(Lists)
        UL(
            LI(Bullet item one)
            LI(Bullet item two)
        )
        OL(
            LI(Numbered item primary)
            LI(Numbered item secondary)
        )

        H2(Code Blocks)
        CODE(
            RAW(0, #include <stdio.h>)
            RAW(0, )
            RAW(0, int main(void) {)
            RAW(4, puts("Running cmacmd test suite...");)
            RAW(4, return 0;)
            RAW(0, })
        )

        H2(Media AMP() Void Elements)
        PARA(Embedded SVG vector asset generated via byte array:)
        IMG("out/assets/black.svg", "Black Square Graphic", CLASS(hero) ATTR(width, "60"))
    );

    render(&out, raw.str);
    puts(out.str);

    int errors = validate_html(out.str);
    if(errors){
        fprintf(stderr, "html validation failed with %d errors\n", errors);
    }

    free(raw.str);
    free(out.str);
    return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
