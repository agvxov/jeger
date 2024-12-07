#ifndef JEGER_H
#define JEGER_H

typedef struct {
    int state;
    char * pattern;
    char * code;
} rule_t;

extern rule_t * patterns;
extern int alphabet_size;

extern void generate(const char * filename);

#endif
