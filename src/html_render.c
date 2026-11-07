#include <string.h>

#include "html_render.h"
#include "internal.h"

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

static char *read_whole_file(const char *path, size_t *n){
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
    char *buf = read_whole_file(path, &n);
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
    size_t rb = strlen(RB), gb = strlen(GB), ge = strlen(GE);
    size_t ib = strlen(IB), ie = strlen(IE), fb = strlen(FB), fe = strlen(FE);
    (void)RE; // the RAW end marker is matched without its leading space (see below)

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
            const char *end = strstr(in, IE);
            if(!end) ERROR("unterminated CODEINLINE");
            append_escaped_html_string(out, in, (size_t)(end - in));
            in = end + ie;
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

            const char *fend = strstr(p, FE);
            if(!fend) ERROR("unterminated CODEFILE");
            append_file_range(out, path, b, e, mode);
            in = fend + fe;
        }
        else {
            append_char(out, *in++);
        }
    }
}
