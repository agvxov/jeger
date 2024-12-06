// @BAKE gcc -o $*.out $@ -ggdb
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define INITIALIZE_ARRAY(a, n, x) do {\
        for (int i = 0; i < n; i++) { \
            a[i] = x;                 \
        }                             \
    } while (0)
#define INITIALIZE_MATRIX(a, n, m, x) do {\
        for (int i = 0; i < n; i++) {     \
            for (int h = 0; h < m; h++) { \
                a[i][h] = x;              \
            }                             \
        }                                 \
    } while (0)

//#define AS_SYMBOL(c) (c-'a')
#define AS_SYMBOL(c) c
#define ALPHABET_SIZE 128
#define TOKEN_OFFSET ALPHABET_SIZE

typedef struct {
    int state;
    const char * pattern;
} pattern_t;

static inline
void put_header(FILE * f, const int alphabet_size, const int n_states, const int no_match) {
    fputs(
        "#define AS_SYMBOL(c) c\n", // (c-'a')\n
        f
    );
    fprintf(
        f,
        "#define ALPHABET_SIZE %d\n",
        alphabet_size
    );
    fprintf(
        f,
        "#define N_STATES      %d\n",
        n_states
    );
    fprintf(
        f,
        "#define NO_MATCH      %d\n",
        no_match
    );

    fputs("\n", f);
}

static inline
void put_table(FILE * f, const int * table, char * * prefixes, int n_states, int alphabet_size) {
    fputs("int table[N_STATES][ALPHABET_SIZE] = {\n", f);
    for (int i = 0; i < n_states; i++) {
        fprintf(f, "\t[%d] = {", i);
        for (int h = 0; h < ALPHABET_SIZE; h++) {
            if (h == '\\') {
                fprintf(f, "['\\\\'] = %d, ", table[i*ALPHABET_SIZE + h]);
            } else
            if (h == '\'') {
                fprintf(f, "['\\''] = %d, ", table[i*ALPHABET_SIZE + h]);
            } else
            if (isprint(h)) {
                fprintf(f, "['%c'] = %d, ", h, table[i*ALPHABET_SIZE + h]);
            } else {
                fprintf(f, "[%d] = %d, ", h, table[i*ALPHABET_SIZE + h]);
            }
        }
        fprintf(f, "}, /* \"%s\" */\n", prefixes[i]); // XXX can break
    }
    fputs("};\n", f);
}

void put_state_table(int * states, int n) {
    puts("int state_table[] = {");
    for (int i = 0; i < n; i++) {
        if (states[i] == -1) { break; }
        printf("\t[%d] = %d,\n", i, states[i]);
    }
    puts("};");
}

int get_most_common_prefix(const char * pattern, char * * prefixes, int current_state_start) {
    int r = current_state_start;
    for (int i = current_state_start; prefixes[i] != NULL; i++) {
        if (!strncmp(pattern, prefixes[i], strlen(prefixes[i]))) {
            r = i;
        }
    }
    return r;
}

int get_max_number_of_states(const pattern_t * patterns) {
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

void generate(const pattern_t * patterns) {
    // Init
    int n_states = get_max_number_of_states(patterns);

    int states[n_states];
    INITIALIZE_ARRAY(states, n_states, -1);
    states[0] = 0;

    char * prefixes[n_states];
    INITIALIZE_ARRAY(prefixes, n_states, NULL);

    int table[n_states][ALPHABET_SIZE];
    INITIALIZE_MATRIX(table, n_states, ALPHABET_SIZE, TOKEN_OFFSET);

    // Construct table
    int next_free_slot = 1;
    for (
      int pattern_index = 0;
      patterns[pattern_index].pattern != NULL;
      pattern_index++
    ) {
        const pattern_t * pattern = &patterns[pattern_index];

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

        put_table(stderr, (int*)table, prefixes, n_states, ALPHABET_SIZE);
        fputs("/* ================== */\n", stderr);
    }

    /* `get_max_number_of_states()` most likely over estimated,
     *   so we cut back the table to the number of rows that were actually used.
     */
    n_states = next_free_slot;

    // Output
    put_header(stdout, ALPHABET_SIZE, n_states, TOKEN_OFFSET);
    put_table(stdout, (int*)table, prefixes, n_states, ALPHABET_SIZE);
    put_state_table(states, n_states);
}

signed main(void) {
    /*
    pattern_t patterns[] = {
        {0, "while"},
        {0, "printf"},
        {0, "\""},
        {1, "."},
        {1, "\""},
        {0, NULL}
    };
    */
    pattern_t patterns[] = {
        {0, "auto"},
        {0, "break"},
        {0, "case"},
        {0, "char"},
        {0, "const"},
        {0, "continue"},
        {0, "default"},
        {0, "do"},
        {0, "double"},
        {0, "else"},
        {0, "enum"},
        {0, "extern"},
        {0, "float"},
        {0, "for"},
        {0, "goto"},
        {0, "if"},
        {0, "inline"},
        {0, "int"},
        {0, "long"},
        {0, "register"},
        {0, "return"},
        {0, "restrict"},
        {0, "short"},
        {0, "signed"},
        {0, "sizeof"},
        {0, "static"},
        {0, "struct"},
        {0, "switch"},
        {0, "typedef"},
        {0, "union"},
        {0, "unsigned"},
        {0, "void"},
        {0, "volatile"},
        {0, "while"},
        {0, " "},
        {0, "\n"},
        {0, "("},
        {0, ")"},
        {0, "{"},
        {0, "}"},
        {0, "["},
        {0, "]"},
        {0, ","},
        {0, ";"},
        {0, "\""},
        {0, "/*"},
        {0, "//"},
        {1, "\\\""},
        {1, "\""},
        {2, "*/"},
        {3, "\n"},
        {0, NULL}
    };
    //pattern_t patterns[] = {
    //    {0, "short"},
    //    {0, "signed"},
    //    {0, "sizeof"},
    //    {0, "static"},
    //    {0, "struct"},
    //    {0, "switch"},
    //    {0, NULL}
    //};

    generate(patterns);

    return 0;
}

/*
            if (pattern->pattern[i] == '.') {
                for (int col = 0; col < ALPHABET_SIZE; col++) {
                    table
                        [next_free_slot]
                        [col]
                      = next_free_slot + 1
                    ;
                }
            } else {
                table
                    [next_free_slot]
                    [AS_SYMBOL(pattern->pattern[i])]
                  = next_free_slot + 1
                ;
            }
*/
