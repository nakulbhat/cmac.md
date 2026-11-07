#ifndef HTML_INTERNAL_H
#define HTML_INTERNAL_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define ERROR(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE);} while(0)
#define ABORT(...) do { fprintf(stderr, __VA_ARGS__); abort();} while(0)

#endif
