#ifndef JEGER_H
#define JEGER_H

typedef struct {
    int state;
    const char * pattern;
} pattern_t;

extern pattern_t * patterns;
extern int alphabet_size;

extern void generate(const char * filename);

#endif
