// @BAKE gcc -o $*.out $@ -ggdb
#include <stdio.h>

/*
#define ALPHABET_SIZE 4
#define N_STATES      3
#define AS_SYMBOL(c) (c-'a')

enum {
    NO_MATCH = 100,
    AAB_MATCH,
    AAC_MATCH,
};

char table[N_STATES][ALPHABET_SIZE] = {
    [0] = {1, NO_MATCH, NO_MATCH, NO_MATCH},
    [1] = {2, NO_MATCH, NO_MATCH, NO_MATCH},
    [2] = {NO_MATCH, AAB_MATCH, AAC_MATCH, NO_MATCH},
};
*/

int had_seperation = 1;

#include "generated.h"

int mlen;

static inline
int mlookup(const char * s, int state) {
    for (int i = 0; s[i] != '\0'; i++) {
        state = table[state][AS_SYMBOL(s[i])];
        if (state == NO_MATCH) {
            break;
        } else
        if (state > NO_MATCH) {
            mlen = i+1;
            return state;
        }
    }

    mlen = 0;
    return NO_MATCH;
}

#define N_KEYWORDS   34
#define N_SEPARATORS 10
#define RETARDATION_OFFSET (NO_MATCH+N_KEYWORDS+N_SEPARATORS)

#if 0
#  define TRACE fprintf(stderr, "--accepting rule at line %d (\"%.*s\")\n", __LINE__, mlen, ss);
#  define TRACE_DEFAULT fprintf(stderr, "--accepting default rule (\"%c\")\n", *ss);
#else
#  define TRACE
#  define TRACE_DEFAULT
#endif

int mlex(const char * s) {
    int state = 0;
    for (const char * ss = s; *ss != '\0'; ss += (mlen ? mlen : 1)) {
        int match = mlookup(ss, state_table[state]);
        if (match != NO_MATCH) {
            
        } else {
            
        }
        switch (match) {
            case NO_MATCH: {
                TRACE_DEFAULT;
                putchar(*ss);
                had_seperation = 0;
            } break;
            // keyword
            case NO_MATCH+1 ... NO_MATCH+N_KEYWORDS: {
                TRACE;
                if (had_seperation) {
                    printf("\033[31m%.*s\033[0m", mlen, ss);
                } else {
                    printf("%.*s", mlen, ss);
                }
                had_seperation = 0;
            } break;
            // Sep
            case NO_MATCH+N_KEYWORDS+1 ... RETARDATION_OFFSET: {
                TRACE;
                printf("\033[35m%c\033[0m", *ss);
                //putchar(*ss);
                had_seperation = 1;
            } break;
            // string
            case RETARDATION_OFFSET+1: {
                TRACE;
                state = 1;
                printf("\033[32m\"");
            } break;
            case RETARDATION_OFFSET+5: {
                TRACE;
                state = 0;
                printf("\"\033[0m");
            } break;
            // comment (multiline)
            case RETARDATION_OFFSET+2: {
                TRACE;
                state = 2;
                printf("\033[34m/*");
            } break;
            case RETARDATION_OFFSET+6: {
                TRACE;
                state = 0;
                printf("*/\033[0m");
                had_seperation = 1;
            } break;
            // comment (single line)
            case RETARDATION_OFFSET+3: {
                TRACE;
                state = 3;
                printf("\033[34m//");
            } break;
            case RETARDATION_OFFSET+7: {
                TRACE;
                state = 0;
                printf("\033[0m\n");
                had_seperation = 1;
            } break;
        }
    }
    return 0;
}

extern const char * source_code;

signed main(void) { 
    //mlex("while (1) { printf(\"Heyo\"); }\n");
    mlex(source_code);

    return 0;
}
