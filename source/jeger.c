#ifdef __cplusplus
# pragma GCC diagnostic ignored "-Wc++20-extensions"
#endif

#include "jeger.h"

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#if DEBUG
# include <stdio.h>
#endif

#define JEGER_SOS_STATE   0
#define JEGER_NSOS_STATE  1
#define JEGER_INIT_STATE  2

// ------------------
// ### Char tests ###
// ------------------
static inline
bool mystrchr(const char * const str, const char c){
	for (const char * s = str; *s != '\00'; s++) {
		if (*s == c) {
			return true;
		}
	}
	return false;
}

static inline
bool is_quantifier(const char c) {
	return mystrchr("=?+*", c);
}

static inline
bool is_hologram_escape(const char c) {
	return mystrchr("<>", c);
}

bool is_magic(const char c) {
	return is_quantifier(c)
	    || mystrchr("\\[].^", c)
		;
}

// -------------------
// ### Match tests ###
// -------------------
static inline
bool is_sentinel(const match_t * const match) {
	return (match->position == -1)
	    && (match->width    == -1)
	    ;
}

// -----------------
// ### Char sets ###
// -----------------
#define JEGER_CHAR_SET_at                "@"
#define JEGER_CHAR_SET_underscore        "_"
#define JEGER_CHAR_SET_lower             "abcdefghijklmnopqrstuwxyz"
#define JEGER_CHAR_SET_upper             "ABCDEFGHIJKLMNOPQRSTUWXYZ"
#define JEGER_CHAR_SET_digits            "0123456789"
#define JEGER_CHAR_SET_octal_digits      "01234567"
#define JEGER_CHAR_SET_lower_hex         "abcdef"
#define JEGER_CHAR_SET_upper_hex         "ABCDEF"
#define JEGER_CHAR_SET_oct_241_to_277                           \
			                             "\241\242\243\244\245" \
			                             "\246\247\250\251\252" \
			                             "\253\254\255\256\257" \
			                             "\260\261\262\263\264" \
			                             "\265\266\267\270\271" \
			                             "\272\273\274\275\276" \
			                             "\277"
#define JEGER_CHAR_SET_oct_300_to_337                           \
                                         "\300\301\302\303\304" \
                                         "\305\306\307\310\311" \
                                         "\312\313\314\315\316" \
                                         "\317\320\321\322\323" \
                                         "\324\325\326\327\330" \
                                         "\331\332\333\334\335" \
                                         "\336\337"
#define JEGER_CHAR_SET_file_extra        "/.-_+,#$%~="
#define JEGER_CHAR_SET_whitespace        " " "\t\v\n"

static const char JEGER_CHAR_symbol_chars[] = 
                                             JEGER_CHAR_SET_underscore
                                             JEGER_CHAR_SET_lower
                                             JEGER_CHAR_SET_upper
                                         ;

// ----------------------
// ### Internal Types ###
// ----------------------
typedef struct {
	int in;
	char input;
	int to;
	int pattern_width;
	int match_width;
} delta_t;

typedef struct {
	int in;
	int to;
	int pattern_width;
	int match_width;
} offshoot_t;

enum {
	DO_CATCH                  = 0x00000001 << 0,
	IS_NEGATIVE               = 0x00000001 << 1,
	IS_AT_THE_BEGINNING       = 0x00000001 << 2,
	FORCE_START_OF_STRING     = 0x00000001 << 3,
	DO_FORBID_START_OF_STRING = 0x00000001 << 4,
	INCREMENT_STATE           = 0x00000001 << 5,
};

typedef struct {
	int       flags;
	int       state;
	int       width;
	char    * whitelist;
	char    * blacklist;
} compiler_state;



// ----------------------------------
// ### Regex creation/destruction ###
// ----------------------------------
static const int HALT_AND_CATCH_FIRE = INT_MIN;

#define ASSERT_HALT(a) ((a == HALT_AND_CATCH_FIRE) ? HALT_AND_CATCH_FIRE : (cs->state + a))

static
void HOOK_ALL(const int                         from,
              const char           * const       str,
              const int                           to,
              const compiler_state * const        cs,
					regex_t        *           regex) {
	for (const char * s = str; *s != '\0'; s++) {
		delta_t * delta = (delta_t *)malloc(sizeof(delta_t));
		*delta = (delta_t){
			.in            = cs->state + from,
			.input         = *s,
			.to            = ASSERT_HALT(to),
			.pattern_width = cs->width,
			.match_width   = 1,
		};
		vector_push(&regex->delta_table,
		            &delta);
	}
}

static
void ABSOLUTE_OFFSHOOT(const int              from,
                       const int                to,
                       const int             width,
                       const int       match_width,
					         regex_t *       regex) {
	offshoot_t * offshoot = (offshoot_t *)malloc(sizeof(offshoot_t));
	*offshoot = (offshoot_t){
		.in            = from,
		.to            = to,
		.pattern_width = width,
		.match_width   = match_width,
	};
	vector_push(&regex->catch_table,
	            &offshoot);
}

static
void OFFSHOOT(const int                     from,
              const int                       to,
              const int                    width,
              const int              match_width,
              const compiler_state *          cs,
					regex_t        *       regex) {
	ABSOLUTE_OFFSHOOT(cs->state + from, ASSERT_HALT(to), width, match_width, regex);
}

static
int escape_1_to_1(const char                    c,
                  const compiler_state * const cs) {
	char * target_list = (cs->flags & IS_NEGATIVE) ? cs->blacklist : cs->whitelist;
	switch (c) {
		case 't': {
			strcat(target_list, "\t");
		} return 1;
		case 'n': {
			strcat(target_list, "\n");
		} return 1;
		case 'r': {
			strcat(target_list, "\r");
		} return 1;
		case 'b': {
			strcat(target_list, "\b");
		} return 1;
		case '[': {
			strcat(target_list, "[");
		} return 1;
		case ']': {
			strcat(target_list, "]");
		} return 1;
		case '.': {
			strcat(target_list, ".");
		} return 1;
		case '^': {
			strcat(target_list, "^");
		} return 1;
		case '=': {
			strcat(target_list, "=");
		} return 1;
		case '?': {
			strcat(target_list, "?");
		} return 1;
		case '+': {
			strcat(target_list, "+");
		} return 1;
		case '*': {
			strcat(target_list, "*");
		} return 1;
		case '\\': {
			strcat(target_list, "\\");
		} return 1;
	}

	return 0;
}

static
int escape_1_to_N(const char                    c,
                  const compiler_state * const cs) {
	char * target_list = (cs->flags & IS_NEGATIVE) ? cs->blacklist : cs->whitelist;
	switch(c) {
		case 'i': {
			const char identifier_chars[] = JEGER_CHAR_SET_at
			                                JEGER_CHAR_SET_underscore
			                                JEGER_CHAR_SET_digits
			                                JEGER_CHAR_SET_oct_300_to_337
			                              ;
			strcpy(target_list, identifier_chars);
			return sizeof(identifier_chars)-1;
		};
		case 'I': {
			const char identifier_chars[] = JEGER_CHAR_SET_at
			                                JEGER_CHAR_SET_underscore
			                                JEGER_CHAR_SET_oct_300_to_337
			                              ;
			strcpy(target_list, identifier_chars);
			return sizeof(identifier_chars)-1;
		};
		case 'k': {
			const char keyword_chars[] = JEGER_CHAR_SET_at
			                             JEGER_CHAR_SET_underscore
			                             JEGER_CHAR_SET_digits
			                             JEGER_CHAR_SET_oct_300_to_337
			                           ;
			strcpy(target_list, keyword_chars);
			return sizeof(keyword_chars)-1;
		};
		case 'K': {
			const char keyword_chars[] = JEGER_CHAR_SET_at
			                             JEGER_CHAR_SET_underscore
			                             JEGER_CHAR_SET_oct_300_to_337
			                           ;
			strcpy(target_list, keyword_chars);
			return sizeof(keyword_chars)-1;
		};
		case 'f': {
			const char filename_chars[] = JEGER_CHAR_SET_at
			                              JEGER_CHAR_SET_digits
			                              JEGER_CHAR_SET_file_extra
			                            ;
			strcpy(target_list, filename_chars);
			return sizeof(filename_chars)-1;
		};
		case 'F': {
			const char filename_chars[] = JEGER_CHAR_SET_at
			                              JEGER_CHAR_SET_file_extra
			                            ;
			strcpy(target_list, filename_chars);
			return sizeof(filename_chars)-1;
		};
		case 'p': {
			const char printable_chars[] = JEGER_CHAR_SET_at
			                               JEGER_CHAR_SET_oct_241_to_277
			                               JEGER_CHAR_SET_oct_300_to_337
			                             ;
			strcpy(target_list, printable_chars);
			return sizeof(printable_chars)-1;
		};
		case 'P': {
			const char printable_chars[] = JEGER_CHAR_SET_at
			                               JEGER_CHAR_SET_oct_241_to_277
			                               JEGER_CHAR_SET_oct_300_to_337
			                             ;
			strcpy(target_list, printable_chars);
			return sizeof(printable_chars)-1;
		};
		case 's': {
			const char whitespace_chars[] = JEGER_CHAR_SET_whitespace;
			strcpy(target_list, whitespace_chars);
			return sizeof(whitespace_chars)-1;
		};
		case 'd': {
			const char digit_chars[] = JEGER_CHAR_SET_digits;
			strcpy(target_list, digit_chars);
			return sizeof(digit_chars)-1;
		};
		case 'x': {
			const char hex_chars[] = JEGER_CHAR_SET_digits
			                         JEGER_CHAR_SET_lower_hex
			                         JEGER_CHAR_SET_upper_hex
			                       ;
			strcpy(target_list, hex_chars);
			return sizeof(hex_chars)-1;
		};
		case 'o': {
			const char oct_chars[] = JEGER_CHAR_SET_octal_digits;
			strcpy(target_list, oct_chars);
			return sizeof(oct_chars)-1;
		};
		case 'w': {
			const char word_chars[] = JEGER_CHAR_SET_underscore
			                          JEGER_CHAR_SET_digits
			                          JEGER_CHAR_SET_lower
			                          JEGER_CHAR_SET_upper
			                        ;
			strcpy(target_list, word_chars);
			return sizeof(word_chars)-1;
		};
		case 'h': {
			// #global JEGER_CHAR_symbol_chars
			strcpy(target_list, JEGER_CHAR_symbol_chars);
			return sizeof(JEGER_CHAR_symbol_chars)-1;
		};
		case 'a': {
			const char alpha_chars[] = JEGER_CHAR_SET_lower
			                           JEGER_CHAR_SET_upper
			                         ;
			strcpy(target_list, alpha_chars);
			return sizeof(alpha_chars)-1;
		};
		case 'l': {
			const char lower_alpha_chars[] = JEGER_CHAR_SET_lower;
			strcpy(target_list, lower_alpha_chars);
			return sizeof(lower_alpha_chars)-1;
		};
		case 'u': {
			const char upper_alpha_chars[] = JEGER_CHAR_SET_upper;
			strcpy(target_list, upper_alpha_chars);
			return sizeof(upper_alpha_chars)-1;
		};
	}

	return 0;
}

static inline
int escape_to_negative(const char                    c,
                             compiler_state * const cs) {
	switch (c) {
		case 'D': {
			const char digit_chars[] = JEGER_CHAR_SET_digits;
			strcpy(cs->blacklist, digit_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(digit_chars)-1;
		};
		case 'X': {
			const char hex_chars[] = JEGER_CHAR_SET_digits
			                         JEGER_CHAR_SET_lower_hex
			                         JEGER_CHAR_SET_upper_hex
			                       ;
			strcpy(cs->blacklist, hex_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(hex_chars)-1;
		};
		case 'O': {
			const char oct_chars[] = JEGER_CHAR_SET_octal_digits;
			strcpy(cs->blacklist, oct_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(oct_chars)-1;
		};
		case 'W': {
			const char word_chars[] = JEGER_CHAR_SET_underscore
			                          JEGER_CHAR_SET_digits
			                          JEGER_CHAR_SET_lower
			                          JEGER_CHAR_SET_upper
			                        ;
			strcpy(cs->blacklist, word_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(word_chars)-1;
		};
		case 'L': {
			const char lower_alpha_chars[] = JEGER_CHAR_SET_lower;
			strcpy(cs->blacklist, lower_alpha_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(lower_alpha_chars)-1;
		};
		case 'U': {
			const char upper_alpha_chars[] = JEGER_CHAR_SET_upper;
			strcpy(cs->blacklist, upper_alpha_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(upper_alpha_chars)-1;
		};
	}

	return 0;
}

static inline
int compile_dot(compiler_state * const cs) {
	cs->flags |= DO_CATCH;
	return true;
}

static inline
int compile_escape(const char                    c,
                         compiler_state * const cs) {

	return escape_1_to_1(c,      cs)
	    || escape_1_to_N(c,      cs)
	    || escape_to_negative(c, cs)
	    ;
}

static
int compile_range(const char           * const range,
                        compiler_state * const    cs) {
	assert((range[0] == '[') && "Not a range.");

	const char * s;
	if (range[1] == '^') {
		cs->flags |= IS_NEGATIVE;
		s = range + 2;
	} else {
		s = range + 1;
	}

	char * target_list = (cs->flags & IS_NEGATIVE) ? cs->blacklist : cs->whitelist;

	for (; *s != ']'; s++) {
		assert((*s != '\0') && "Unclosed range.");
		char c = *s;
		if (c == '\\') {
			s += 1;
			assert(compile_escape(*s, cs) && "Unknown escape.");
		} else if (*(s+1) == '-') {
			char end = *(s+2);
			assert((c < end) && "Endless range.");
			for (char cc = c; cc < end+1; cc++) {
				strncat(target_list,  &cc, 1);
				strncat(target_list, "\0", 1);
			}
			s += 2;
		} else {
			strncat(target_list,   &c, 1);
		}
	}

	return ((s - range) + 1);
}

static
void filter_blacklist(const char * whitelist,
                      const char * blacklist,
                            char *  filtered) {
	for (; *blacklist != '\0'; blacklist++) {
		for (; *whitelist != '\0'; whitelist++) {
			if (*blacklist == *whitelist) {
				goto long_continue;
			}
		}
		strncat(filtered, blacklist, 1);
		long_continue:
		;
	}
}

regex_t * regex_compile(const char * const pattern) {
	regex_t * regex = (regex_t *)malloc(sizeof(regex_t));
	regex->str = strdup(pattern);
	vector_init(&regex->delta_table, sizeof(delta_t*), 0UL);
	vector_init(&regex->catch_table, sizeof(offshoot_t*), 0UL);

	char whitelist[64];
	char blacklist[64];

	compiler_state cs = {
		.flags     = IS_AT_THE_BEGINNING,
		.state     = JEGER_INIT_STATE,
		.whitelist = whitelist,
		.blacklist = blacklist,
	};

	for (const char * s = pattern; *s != '\00';) {
		assert(!is_quantifier(*s) && "Pattern starts with quantifier.");
		// Reset the compiler
		whitelist[0] = '\0';
		blacklist[0] = '\0';
		cs.flags    &= (IS_AT_THE_BEGINNING | FORCE_START_OF_STRING);
		cs.width     = 1;

		// Translate char
		switch (*s) {
			case '^': {
				;
			} break;
			case '.': {
				compile_dot(&cs);
				s += 1;
			} break;
			case '\\': {
				s += 1;
				if (compile_escape(*s, &cs)) {
					s += 1;
				} else if (is_hologram_escape(*s)) {
					s -= 1;
				} else {
					assert("Unknown escape.");
				}
			} break;
			case '[': {
				s += compile_range(s, &cs);
			} break;
			default: { // Literal
				whitelist[0] =   *s;
				whitelist[1] = '\0';
				s += 1;
			} break;
		}

		/* Ew */
		if (*s == '\\'
		&&  is_hologram_escape(*(s+1))) {
			++s;
		}

		// Compile char
		switch (*s) {
			// holograms
			case '^': {
				whitelist[0] = '\n';
				whitelist[1] = '\0';
				HOOK_ALL(0, whitelist, 0, &cs, regex);
				if (cs.flags & IS_AT_THE_BEGINNING) {
					cs.flags |= FORCE_START_OF_STRING;
				} else {
					cs.flags |= INCREMENT_STATE;
				}
				s += 1;
			} break;
			case '<': {
				// XXX: make this legible
				if (cs.flags & IS_AT_THE_BEGINNING
				&& !(cs.flags & DO_CATCH)
				&& !(cs.flags & IS_NEGATIVE)
				&& whitelist[0] == '\0') {
					// ---
					cs.flags |= INCREMENT_STATE;
					cs.flags |= DO_FORBID_START_OF_STRING;
					strcat(whitelist, JEGER_CHAR_symbol_chars);
					// ---
					ABSOLUTE_OFFSHOOT( JEGER_SOS_STATE, JEGER_INIT_STATE+1, 0, 0, regex);
					ABSOLUTE_OFFSHOOT(JEGER_INIT_STATE, JEGER_INIT_STATE+2, 1, 0, regex);
					HOOK_ALL(0, whitelist, HALT_AND_CATCH_FIRE, &cs, regex);
					// ---
					++cs.state;
					cs.width = 0;
					HOOK_ALL(0, whitelist, +1, &cs, regex);
					cs.width = 1;
					OFFSHOOT(0, +1, 1, 0, &cs, regex);
					// ---
				} else {
					HOOK_ALL(0, whitelist, +1, &cs, regex);
					if ((cs.flags & DO_CATCH)
					||  (cs.flags & IS_NEGATIVE)) {
						OFFSHOOT(+1, +2, 1, 1, &cs, regex);
					} else {
						cs.flags |= INCREMENT_STATE;
					}
					OFFSHOOT(0, +1, 1, 0, &cs, regex);
				}
				cs.flags |= IS_NEGATIVE;
				strcat(blacklist, JEGER_CHAR_symbol_chars);
				s += 1;
			} break;
			case '>': {
				HOOK_ALL(0, whitelist, +1, &cs, regex);
				cs.flags |= IS_NEGATIVE | INCREMENT_STATE;
				strcat(blacklist, JEGER_CHAR_symbol_chars);
				OFFSHOOT(+1, +2, 0, 0, &cs, regex); 
				++cs.state;
				s += 1;
			} break;
			// quantifiers
			case '=':
			case '?': {
				HOOK_ALL(0, whitelist, +1, &cs, regex);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs, regex);
				}
				s += 1;
			} break;
			case '*': {
				HOOK_ALL(0, whitelist,  0, &cs, regex);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, 0, 1, 1, &cs, regex);
				}
				s += 1;
			} break;
			case '+': {
				cs.flags |= INCREMENT_STATE;
				HOOK_ALL(0, whitelist, +1, &cs, regex);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs, regex);
				}
				HOOK_ALL(+1, whitelist, +1, &cs, regex);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(+1, +1, 1, 1, &cs, regex);
				}
				s += 1;
			} break;
			default: { // Literal
				cs.flags |= INCREMENT_STATE;
				HOOK_ALL(0, whitelist, +1, &cs, regex);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs, regex);
				}
			} break;
		}

		// Compile blacklist
		if (*blacklist) {
			char filtered_blacklist[64];
			filtered_blacklist[0] = '\0'; 
			filter_blacklist(whitelist, blacklist, filtered_blacklist);
			HOOK_ALL(0, filtered_blacklist, HALT_AND_CATCH_FIRE, &cs, regex);
		}

		if (cs.flags & INCREMENT_STATE) {
			++cs.state;
		}

		cs.flags &= (~IS_AT_THE_BEGINNING);
	}

	// Init state hookups
	if (!(cs.flags & DO_FORBID_START_OF_STRING)) {
		ABSOLUTE_OFFSHOOT(JEGER_SOS_STATE, JEGER_INIT_STATE, 0, 0, regex);
	}
	if (cs.flags & FORCE_START_OF_STRING) {
		ABSOLUTE_OFFSHOOT(JEGER_NSOS_STATE, HALT_AND_CATCH_FIRE, 0, 0, regex);
	} else {
		ABSOLUTE_OFFSHOOT(JEGER_NSOS_STATE,    JEGER_INIT_STATE, 0, 0, regex);
	}

	regex->accepting_state = cs.state;

	return regex;
}

int regex_free(regex_t * const regex) {
	free(regex->str);
	vector_free(&regex->delta_table);
	vector_free(&regex->catch_table);
	free(regex);
	return 0;
}



// -----------------
// ### Searching ###
// -----------------
static
const offshoot_t * catch_table_lookup(const regex_t * const regex,
                                      const int     * const state) {
	for (size_t i = 0; i < regex->catch_table.element_count; i++){
		const offshoot_t * const offshoot = *(offshoot_t**)vector_get(&regex->catch_table, i);
		if (offshoot->in == *state) {
			return offshoot;
		}
	}
	return NULL;
}

static
bool regex_assert(const regex_t * const         regex,
                  const char    * const        string,
                        int                     state,
                        match_t * const         match) {
	if (state == HALT_AND_CATCH_FIRE) {
		return false;
	}

	bool last_stand = false;
	bool was_found;

	const char * s = string;
	LOOP: {
		was_found = false;
		if (*s == '\0') {
			last_stand = true;
			goto PERFORM_CATCH_LOOKUP;
		}
		// Jump search for the correct state
		const int jump = 10;
		size_t i = jump;
		while (i < regex->delta_table.element_count) {
			const delta_t * const delta = *(delta_t**)vector_get(&regex->delta_table, i);
			if (delta->in >= state) {
				break;
			}
			i += jump;
		}
		i -= jump;
		// Linear search finish up
		for (; i < regex->delta_table.element_count; i++) {
			const delta_t * const delta = *(delta_t**)vector_get(&regex->delta_table, i);

			if (delta->in > state) {
				break;
			}

			if ((delta->in == state) 
			&&  (delta->input == *s)) {
				was_found = true;
				const int r = regex_assert(regex, s + delta->pattern_width, delta->to, match);
				if(r){
					match->width += delta->match_width;
					return r;
				}
			}
		}
	}

	PERFORM_CATCH_LOOKUP: {
		if (!was_found) {
			const offshoot_t * const my_catch = catch_table_lookup(regex, &state);
			if (my_catch && (!my_catch->pattern_width || !last_stand)) {
				state = my_catch->to;
				s += my_catch->pattern_width;
				if (match->position < 1) {
					match->position = my_catch->match_width;
				}
				match->width += my_catch->match_width;
				goto LOOP;
			}
		}
	}

	return (state == regex->accepting_state);
}

match_t * regex_match(const regex_t * const              regex,
                      const char    * const             string,
                      const bool            is_start_of_string) {

	vector_t matches;
	vector_init(&matches, sizeof(match_t), 0);

	match_t * match = (match_t *)malloc(sizeof(match_t));

	/* Non-existent regex does not match anything.
	 * Not to be confused with an empty regex.
	 */
	if (regex == NULL) {
		goto FINISH;
	}

	// Find all matches
	{
		const char * s = string;
		int initial_state;
		do {
			initial_state = (int)(!(is_start_of_string && (s == string)));

			*match = (match_t){
				.position = -1,
				.width    =  0,
			};

			if (regex_assert(regex, s, initial_state, match)) {
				if(match->position == -1){
				match->position = (s - string);
				}else{
				match->position += (s - string);
				}

				vector_push(&matches, match);

				s += ((match->width > 0) ? match->width : 1);
				match = (match_t *)malloc(sizeof(match_t));
			} else {
				++s;
			}
		} while (*s != '\0');
	}

	FINISH:

	// Insert sentinel
	*match = (match_t){
		.position = -1,
		.width    = -1,
	};
	vector_push(&matches, match);

	// Hide internal vector usage
	const size_t data_size = matches.element_size * matches.element_count;
	match_t * r = (match_t *)malloc(data_size);
	memcpy(r, matches.data, data_size);
	vector_free(&matches);

	return r;
}

bool regex_search(const regex_t * const  regex,
                  const char    * const string) {

	match_t * m = regex_match(regex, string, true);
	const bool r = !is_sentinel(m);
	free(m);

	return r;
}
