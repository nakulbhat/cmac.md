#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

// Writes `len` bytes to `path`, creating parent directories as needed
// (like mkdir -p). Errors out on failure.
void write_asset(const char *path, const unsigned char *data, size_t len);

// `arr` must be an actual array (not a pointer), so sizeof gives its byte count.
#define ASSET(path, arr)  write_asset(path, arr, sizeof(arr))

#endif
