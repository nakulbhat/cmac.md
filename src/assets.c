#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "assets.h"
#include "internal.h"

// mkdir -p for everything before the last '/' in path
static void make_parent_dirs(const char *path){
    char buf[PATH_MAX];
    size_t n = strlen(path);
    if(n >= sizeof buf) ERROR("ASSET: path too long: %s\n", path);
    memcpy(buf, path, n + 1);

    for(size_t i = 1; i < n; i++){
        if(buf[i] != '/') continue;
        buf[i] = '\0';
        if(mkdir(buf, 0777) != 0 && errno != EEXIST)
            ERROR("ASSET: cannot create directory %s: %s\n", buf, strerror(errno));
        buf[i] = '/';
    }
}

void write_asset(const char *path, const unsigned char *data, size_t len){
    // Reject ".." components so an asset can't escape the output tree
    for(const char *p = path; *p; p++){
        if(p[0] == '.' && p[1] == '.' &&
           (p == path || p[-1] == '/') && (p[2] == '/' || p[2] == '\0'))
            ERROR("ASSET: '..' not allowed in path: %s\n", path);
    }

    make_parent_dirs(path);

    FILE *f = fopen(path, "wb");
    if(!f) ERROR("ASSET: cannot open %s: %s\n", path, strerror(errno));
    if(len && fwrite(data, 1, len, f) != len){
        fclose(f);
        ERROR("ASSET: short write to %s\n", path);
    }
    if(fclose(f) != 0) ERROR("ASSET: error closing %s\n", path);
}
