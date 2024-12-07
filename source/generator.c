#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "jeger.h"
#include "snippets.inc"

//#define AS_SYMBOL(c) (c-'a')
#define AS_SYMBOL(c) ((int)c)
#define TOKEN_OFFSET 128 /* XXX */

int alphabet_size = 128;
rule_t * patterns;

char * definition_section_code_buffer;
char * code_section_code_buffer;

static int n_states = 0;

static inline
void put_header(FILE * f, const int alphabet_size, const int n_states, const int no_match) {
    #define DEFINE_INT(m, n) fprintf(f, "#define " #m " %d\n", n);
    #define DEFINE_STR(m, s) fprintf(f, "#define " #m " %s\n", s);

    DEFINE_INT(ALPHABET_SIZE, alphabet_size);
    DEFINE_INT(N_STATES, n_states);
    DEFINE_INT(NO_MATCH, no_match);
    DEFINE_STR(REVERSE, "(direction *= -1)");
    fputs("#define AS_SYMBOL(c) c\n", /* (c-'a')\n */ f);

    // XXX make this conditional
    DEFINE_STR(TRACE, "fprintf(stderr, \"--accepting rule at line %d (\"%.*s\")\\n\", __LINE__, mlen, ss);");
    DEFINE_STR(TRACE_DEFAULT, "fprintf(stderr, \"--accepting default rule (\"%c\")\\n\", *ss);");
    // DEFINE_STR(TRACE, "");
    // DEFINE_STR(TRACE_DEFAULT, "");
    
    fputs("int mlen;\n", f);

    fputs("\n", f);
}

static inline
void put_table(FILE * f, const int * table, char * * prefixes, int n_states, int alphabet_size) {
    fputs("int table[N_STATES][ALPHABET_SIZE] = {\n", f);
    for (int i = 0; i < n_states; i++) {
        fprintf(f, "\t[%d] = {", i);
        for (int h = 0; h < alphabet_size; h++) {
            /* NOTE: we have to awkwardly escate "\" and "'",
             *        then also print printable characters as themselves
             */
            if (h == '\\') {
                fprintf(f, "['\\\\'] = %d, ", table[i*alphabet_size + h]);
            } else
            if (h == '\'') {
                fprintf(f, "['\\''] = %d, ", table[i*alphabet_size + h]);
            } else
            if (isprint(h)) {
                fprintf(f, "['%c'] = %d, ", h, table[i*alphabet_size + h]);
            } else {
                fprintf(f, "[%d] = %d, ", h, table[i*alphabet_size + h]);
            }
        }
        fprintf(f, "}, /* \"%s\" */\n", prefixes[i]); // XXX can break
    }
    fputs("};\n", f);
}

static
void put_state_table(FILE * f, int * states, int n) {
    fprintf(f, "int state_table[%d] = {\n", n);
    for (int i = 0; i < n; i++) {
        if (states[i] == -1) { break; }
        fprintf(f, "\t[%d] = %d,\n", i, states[i]);
    }
    fputs("};\n\n", f);
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
int get_max_number_of_states(const rule_t * patterns) {
    int r = 0;
    int state_max_accumulator = -1;
    for (int i = 0; patterns[i].pattern != NULL; i++) {
        r += strlen(patterns[i].pattern);
        if (patterns[i].state > state_max_accumulator) {
            state_max_accumulator = patterns[i].state;
            ++r;
        }
    }

    return r;
}

static
void make_and_put_table(FILE * f) {
    // Init
    n_states = get_max_number_of_states(patterns);

    int states[n_states];
    INITIALIZE_ARRAY(states, n_states, -1);
    states[0] = 0;

    char * prefixes[n_states];
    INITIALIZE_ARRAY(prefixes, n_states, NULL);

    int table[n_states][alphabet_size];
    INITIALIZE_MATRIX(table, n_states, alphabet_size, TOKEN_OFFSET);

    // Construct table
    int next_free_slot = 1;
    for (
      int pattern_index = 0;
      patterns[pattern_index].pattern != NULL;
      pattern_index++
    ) {
        const rule_t * pattern = &patterns[pattern_index];

        int current_state_start = states[pattern->state];
        if (current_state_start == -1) {
            current_state_start    = next_free_slot;
            states[pattern->state] = next_free_slot;
            ++next_free_slot;
        }

        int most_common_prefix_state = get_most_common_prefix(
            pattern->pattern,
            prefixes,
            current_state_start
        );

        prefixes[current_state_start] = strdup("");

        int most_common_prefix_index = strlen(prefixes[most_common_prefix_state]);
        const char * last_char = pattern->pattern + most_common_prefix_index;

        table
            [most_common_prefix_state]
            [AS_SYMBOL(pattern->pattern[most_common_prefix_index])]
          = next_free_slot
        ;

        for (
          int i = most_common_prefix_index+1;
          pattern->pattern[i] != '\0';
          i++, next_free_slot++
        ) {
            table
                [next_free_slot]
                [AS_SYMBOL(pattern->pattern[i])]
              = next_free_slot + 1
            ;
            prefixes[next_free_slot] = strndup(pattern->pattern, i);
            last_char = pattern->pattern + i;
        }

        int last_position = (last_char == pattern->pattern
                          || most_common_prefix_index == last_char - pattern->pattern)
                          ? most_common_prefix_state
                          : next_free_slot-1
        ;

        table
            [last_position]
            [AS_SYMBOL(*last_char)]
          = TOKEN_OFFSET+1 + pattern_index
        ;

        put_table(stderr, (int*)table, prefixes, n_states, alphabet_size);
        fputs("/* ================== */\n", stderr);
    }

    /* `get_max_number_of_states()` most likely over estimated,
     *   so we cut back the table to the number of rows that were actually used.
     */
    n_states = next_free_slot;

    // Output
    put_table(f, (int*)table, prefixes, n_states, alphabet_size);
    put_state_table(f, states, n_states);
}

static
void put_functions(FILE * f) {
    fputs(yy_lookup_str, f);

    fputs(yy_lex_str_start, f);
    for (rule_t * rule = patterns; rule->code != NULL; rule++) {
        fprintf(f, "\tcase %ld: {\n" "%s\n" "\t} break;\n", rule - patterns, rule->code);
    }
    fputs(yy_lex_str_end, f);
}

void generate(const char * filename) {
    FILE * f = fopen(filename, "w");

    put_header(f, alphabet_size, n_states, TOKEN_OFFSET);
    make_and_put_table(f);

    fputs(definition_section_code_buffer, f);
    put_functions(f);
    fputs(code_section_code_buffer, f);
}
