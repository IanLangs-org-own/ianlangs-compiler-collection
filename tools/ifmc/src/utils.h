#pragma once

#include <string.h>
#include <assert.h>
#include <stdlib.h>
static inline char* __impl__string_dump(const char *s) {
    if (s == NULL) return NULL;
    
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy == NULL) return NULL;
    
    memcpy(copy, s, len);

    return copy;
}

static inline char* __impl__string_dump_n_bytes(const char *s, const size_t slen) {
    if (s == NULL) return NULL;
    
    size_t len = slen > strlen(s)+1 ? strlen(s)+1 : slen;
    char *copy = malloc(slen+1);
    if (copy == NULL) return NULL;
    
    memcpy(copy, s, len);
    copy[slen] = 0;
    return copy;
}

#define string_dump(s, n) _Generic(n,\
    void*: __impl__string_dump(s), \
    default:  __impl__string_dump_n_bytes(s, (size_t)n) \
)

int create_dir(const char* path, int can_exists);