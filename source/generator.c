#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "opts.h"
#include "jeger.h"

#include <sds.h>
#include <ministach.h>
#include "snippets.inc"

// XXX
//#define AS_SYMBOL(c) (c-'a')
#define AS_SYMBOL(c) ((int)c)
#define TOKEN_OFFSET 128 /* XXX */
// ---

rule_t * rules;
int n_rules = 0;
char * * state_names;
int n_states = 0;
int alphabet_size = 128;

char * definition_section_code_buffer;
char * code_section_code_buffer;

// Code generation microlang
#define DEFINE_INT(m, n) fprintf(f, "#define " #m " %d\n", n);
#define DEFINE_STR(m, s) fprintf(f, "#define " #m " %s\n", s);
#define ECHOF(s, ...)    fprintf(f, s, __VA_ARGS__);
#define ECHO(s, ...)     fputs(s, f);

// ### Generic
static inline
void put_header(FILE * f, const int alphabet_size, const int no_match) {
    DEFINE_INT(ALPHABET_SIZE, alphabet_size);
    DEFINE_INT(N_RULES, n_rules);
    DEFINE_INT(NO_MATCH, no_match);
    DEFINE_STR(BEGIN, "state = ");
    DEFINE_STR(REVERSE, "(direction *= -1)");
    ECHO("#define AS_SYMBOL(c) c\n", /* (c-'a')\n */ f);

    if (do_trace) {
        DEFINE_STR(
            TRACE(l),
            "fprintf(stderr, \"--accepting rule at line %d (\\\"%.*s\\\")\\n\", l, mlen, ss);"
        );
        DEFINE_STR(
            TRACE_DEFAULT,
            "fprintf(stderr, \"--accepting default rule (\\\"%c\\\")\\n\", *ss);"
        );
    } else {
        DEFINE_STR(TRACE, "");
        DEFINE_STR(TRACE_DEFAULT, "");
    }
    
    // XXX we want no globals
    ECHO("int mlen;\n");
    ECHO("int direction = 1;\n");

    ECHO("\n");
}

// ### Table only
static inline
void put_table(FILE * f, const int * table, char * * prefixes, int n_cases, int alphabet_size) {
    ECHO("int table[N_RULES][ALPHABET_SIZE] = {\n");
    for (int i = 0; i < n_cases; i++) {
        ECHOF("\t[%d] = {", i);
        for (int h = 0; h < alphabet_size; h++) {
            /* NOTE: we have to awkwardly escate "\" and "'",
             *        then also print printable characters as themselves
             */
            if (h == '\\') {
                ECHOF("['\\\\'] = %d, ", table[i*alphabet_size + h]);
            } else
            if (h == '\'') {
                ECHOF("['\\''] = %d, ", table[i*alphabet_size + h]);
            } else
            if (isprint(h)) {
                ECHOF("['%c'] = %d, ", h, table[i*alphabet_size + h]);
            } else {
                ECHOF("[%d] = %d, ", h, table[i*alphabet_size + h]);
            }
        }
        ECHOF("}, /* \"%s\" */\n", prefixes[i]); // XXX can break
    }
    ECHO("};\n");
}

static
void put_state_table(FILE * f, int * states) {
    for (int i = 0; i < n_states; i++) {
        ECHOF(
            "#define %s %d\n",
            state_names[i],
            states[i]
        );
    }

    ECHO("\n");
}

static
int get_most_common_prefix(const char * pattern, char * * prefixes, int current_state_start) {
    int r = current_state_start;
    for (int i = current_state_start; prefixes[i] != NULL; i++) {
        if (!strncmp(pattern, prefixes[i], strlen(prefixes[i]))) {
            r = i;
        }
    }
    return r;
}

static
void make_and_put_table(FILE * f) {
    // Init
    int states[n_states];
    INITIALIZE_ARRAY(states, n_states, -1);
    states[0] = 0;

    char * prefixes[n_rules];
    INITIALIZE_ARRAY(prefixes, n_rules, NULL);

    int table[n_rules][alphabet_size];
    INITIALIZE_MATRIX(table, n_rules, alphabet_size, TOKEN_OFFSET);

    // Construct table
    int next_free_slot = 1;
    for (
      int rule_index = 0;
      rules[rule_index].pattern != NULL;
      rule_index++
    ) {
        const rule_t * rule = &rules[rule_index];

        int current_state_start = states[rule->state];
        if (current_state_start == -1) {
            current_state_start = next_free_slot;
            states[rule->state] = next_free_slot;
            ++next_free_slot;
        }

        int most_common_prefix_state = get_most_common_prefix(
            rule->pattern,
            prefixes,
            current_state_start
        );

        prefixes[current_state_start] = strdup("");

        int most_common_prefix_index = strlen(prefixes[most_common_prefix_state]);
        const char * last_char = rule->pattern + most_common_prefix_index;

        table
            [most_common_prefix_state]
            [AS_SYMBOL(rule->pattern[most_common_prefix_index])]
          = next_free_slot
        ;

        for (
          int i = most_common_prefix_index+1;
          rule->pattern[i] != '\0';
          i++, next_free_slot++
        ) {
            table
                [next_free_slot]
                [AS_SYMBOL(rule->pattern[i])]
              = next_free_slot + 1
            ;
            prefixes[next_free_slot] = strndup(rule->pattern, i);
            last_char = rule->pattern + i;
        }

        int last_position = (last_char == rule->pattern
                          || most_common_prefix_index == last_char - rule->pattern)
                          ? most_common_prefix_state
                          : next_free_slot-1
        ;

        table
            [last_position]
            [AS_SYMBOL(*last_char)]
          = TOKEN_OFFSET+1 + rule_index
        ;

        if (do_debug > 1) {
            put_table(stderr, (int*)table, prefixes, n_rules, alphabet_size);
            fputs("/* ================== */\n", stderr);
        }
    }

    const int n_cases = next_free_slot;

    // Output
    put_state_table(f, states);
    put_table(f, (int*)table, prefixes, n_cases, alphabet_size);

    // Deinit
    for (int i = 0; i < n_states; i++) {
        free(prefixes[i]);
    }
}

static
const char * static_stach(const char * k, int l, void * d) {
    return (char*)d;
}

static
void put_functions(FILE * f) {
    sds inner_code = sdsnew("");

    inner_code = sdscatprintf(
        inner_code,
        "\tcase %d: {\n"
            "TRACE_DEFAULT;\n"
        "\t} break;\n",
        TOKEN_OFFSET
    );
    for (rule_t * rule = rules; rule->code != NULL; rule++) {
        inner_code = sdscatprintf(
            inner_code,
            "\tcase %ld: {\n"
                "TRACE(%d);\n"
                "%s\n"
            "\t} break;\n",
            TOKEN_OFFSET + 1 + (rule - rules),
            rule->line,
            rule->code
        );
    }

    ECHO(yy_lookup_inc);

    ministach_t m;
    ministach_compile(&m, yy_lex_inc, static_stach, inner_code);
    char * buffer = (char*)malloc((m.size + 1) * sizeof(char));
    ministach_render(&m, buffer);
    sdsfree(inner_code);

    ECHO(buffer);

    free(buffer);
}


// ### Switch only
static
void make_and_put_switch(FILE * f) {
    fputs("Not implemented.", stderr);
    exit(1);
}

// ### Extern
void generate(const char * filename) {
    FILE * f = fopen(filename, "w");

    switch (table_type) {
        case STATIC_TABLE: {
            put_header(f, alphabet_size, TOKEN_OFFSET);
            ECHO(definition_section_code_buffer);
            // %%
            make_and_put_table(f);
            put_functions(f);
            // %%
            ECHO(code_section_code_buffer);
        } break;
        case SWITCH_TABLE: {
            put_header(f, alphabet_size, TOKEN_OFFSET);
            ECHO(definition_section_code_buffer);
            // %%
            make_and_put_switch(f);
            // %%
            ECHO(code_section_code_buffer);
        } break;
    }
}

void deinit_jeger(void) {
    for (int i = 0; i < n_states; i++) {
        free(state_names[i]);
    }
    for (int i = 0; i < n_rules; i++) {
        free(rules[i].pattern);
        free(rules[i].code);
    }

    n_rules  = 0;
    n_states = 0;
}
