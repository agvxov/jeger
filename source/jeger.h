#ifndef JEGER_H
#define JEGER_H

#include <stdbool.h>

// Structs
typedef struct {
    int state;
    char * pattern;
    int code_index;
    // temp
    char * code;
    int line;
} rule_t;

typedef struct {
    char * code;
    int line;
} code_t;

typedef enum {
    STATIC_TABLE,
    SWITCH_TABLE,
} table_t;

// Globals
extern rule_t * rules;
extern code_t * codes;
extern int n_rules;
extern char * * state_names;
extern int n_states;
extern int alphabet_size;

extern table_t table_type;

extern char * prefix;
extern bool do_setup_lineno;

extern char * definition_section_code_buffer;
extern char * code_section_code_buffer;

// Functions
extern void generate(const char * filename);
extern void deinit_jeger(void);

#endif
