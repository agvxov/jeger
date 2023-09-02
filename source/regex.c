/* XXX:
 *  as it turns out returning a range of match objects is a
 *  high profile performance issue regarding regex, especially when highlighting.
 *  now as it stands we search an array of tokens for every position on a string.
 *  which sounds ok, until one realizes that searching from any position revails a range,
 *  where (future) matches can or cannot be found. meaning we are computing the same thing
 *  repeatedly, practically resulting in a bruteforcing situation where instead of eliminating
 *  certain non-matches, we blindly hammer character by character.
 */

#include "regex.h"

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define JEGER_INIT_STATE    2

// ------------------
// ### Char tests ###
// ------------------
static bool is_quantifier(const char c) {
	for (const char * s = "+*?="; *s != '\00'; s++) {
		if (*s == c) {
			return true;
		}
	}
	return false;
}

bool is_magic(const char c) {
	if (is_quantifier(c)) {
		return true;
	}
	for (const char * s = "\\[].^"; *s != '\00'; s++) {
		if (*s == c) {
			return true;
		}
	}
	return false;
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
#define JEGER_CHAR_SET_whitespace        " \t\v\n"

// ----------------------
// ### Internal Types ###
// ----------------------
typedef struct {
	int in;
	char input;
	int to;
	int width;
	int match_width;
} delta_t;

typedef struct {
	int in;
	int to;
	int width;
	int match_width;
} offshoot_t;

enum {
	DO_CATCH              = 0x00000001 << 0,
	IS_NEGATIVE           = 0x00000001 << 1,
	IS_AT_THE_BEGINNING   = 0x00000001 << 2,
	DO_SKIP               = 0x00000001 << 3,
	FORCE_START_OF_STRING = 0x00000001 << 4,
};

typedef struct {
	// XXX:
	int flags;
// these might be obsolite but im leaving them for now
	bool      do_loop_hook;
	bool      do_follow_hook;
	bool      do_loop_shoot;
	bool      do_follow_shoot;
// ---
	int       state;
	int       width;
	char    * whitelist;
	char    * blacklist;
	regex_t * regex;
} compiler_state;



// ----------------------------------
// ### Regex creation/destruction ###
// ----------------------------------
static const int HALT_AND_CATCH_FIRE = INT_MIN;

#define ASSERT_HALT(a) ((a == HALT_AND_CATCH_FIRE) ? HALT_AND_CATCH_FIRE : (cs->state + a))

static
void HOOK_ALL(      int              from,
              const char * const      str,
                    int                to,
                    compiler_state *   cs) {
	for (const char * s = str; *s != '\0'; s++) {
		delta_t * delta = (delta_t *)malloc(sizeof(delta_t));
		delta->in    = cs->state + from;
		delta->input = *s;
		delta->to    = ASSERT_HALT(to);
		delta->width = cs->width;
		vector_push(&cs->regex->delta_table,
		            &delta);
	}
}

static
void ABSOLUTE_OFFSHOOT(int             from,
                       int               to,
                       int            width,
                       int      match_width,
                       compiler_state *  cs) {
	offshoot_t * offshoot = (offshoot_t *)malloc(sizeof(offshoot_t));
	offshoot->in    = from; 
	offshoot->to    = to;
	offshoot->width = width;
	offshoot->match_width = match_width;
	vector_push(&cs->regex->catch_table,
	            &offshoot);
}

static
void OFFSHOOT(int             from,
              int               to,
              int            width,
              int      match_width,
              compiler_state *  cs) {
	ABSOLUTE_OFFSHOOT(cs->state + from, ASSERT_HALT(to), width, match_width, cs);
}

static
int escape_1_to_1(const char c, compiler_state * cs) {
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
int escape_1_to_N(const char c, compiler_state * cs) {
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
			const char very_word_chars[] = JEGER_CHAR_SET_underscore
			                               JEGER_CHAR_SET_lower
			                               JEGER_CHAR_SET_upper
			                             ;
			strcpy(target_list, very_word_chars);
			return sizeof(very_word_chars)-1;
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

static
int escape_to_negative(const char                    c,
	                         compiler_state * const cs) {
	switch (c) {
		case 'D': {
			const char digit_chars[] = JEGER_CHAR_SET_digits;
			strcpy(cs->blacklist, digit_chars);
			cs->flags |= IS_NEGATIVE;
			return sizeof(digit_chars)-1;
		};
	}

	return 0;
}

static
int compile_hologram(const char                    c,
                           compiler_state * const cs) {
	static
	const char very_word_chars[] = JEGER_CHAR_SET_underscore
	                               JEGER_CHAR_SET_lower
	                               JEGER_CHAR_SET_upper
	                             ;
	switch (c) {
		case '^': {
			cs->whitelist[0] = '\n';
			cs->whitelist[1] = '\0';
			HOOK_ALL(0, cs->whitelist, 0, cs);
			cs->flags |= DO_SKIP;
			if (cs->flags & IS_AT_THE_BEGINNING) {
				cs->flags |= FORCE_START_OF_STRING;
			} else {
				cs->state += 1;
			}
			return 1;
		};
		case '<': {
			if (cs->flags & IS_AT_THE_BEGINNING) {
				ABSOLUTE_OFFSHOOT(0, 3, 0, 0, cs); //XXX: figure out how to move this
			}
			cs->flags |= DO_SKIP;
			cs->flags |= IS_NEGATIVE;
			strcat(cs->blacklist, very_word_chars);
			OFFSHOOT(0, 0, 1, 0, cs); //XXX: figure out how to move this
			++cs->state;
			return sizeof(very_word_chars)-1;
		};
		case '>': {
			cs->flags |= DO_SKIP;
			cs->flags |= IS_NEGATIVE;
			strcat(cs->blacklist, very_word_chars);
			OFFSHOOT(0, 1, 0, 0, cs); //XXX: figure out how to move this
			++cs->state;	// XXX: the current bug arises from the state being increased before the blacklist is hooked
			
			return sizeof(very_word_chars)-1;
		}
	}
	return 0;

}

static
int compile_dot(compiler_state * cs) {
	cs->flags |= DO_CATCH;
	return true;
}

static
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

void filter_blacklist(const char * whitelist,
                      const char * blacklist,
                            char *  filtered) {
	for (; *blacklist != '\0'; blacklist++) {
		for(; *whitelist != '\0'; whitelist++) {
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

	// this is plain retarded
	char whitelist[64];
	char blacklist[64];

	compiler_state cs = {
		.flags               = IS_AT_THE_BEGINNING,
		.state               = JEGER_INIT_STATE,
		.width               = 0,
		.whitelist           = whitelist,
		.blacklist           = blacklist,
		.regex               = regex,
	};

	for (const char * s = pattern; *s != '\00';) {
		// Reset the compiler
		assert(!is_quantifier(*s) && "Pattern starts with quantifier.");
		whitelist[0]             =  '\0';
		blacklist[0]             =  '\0';
		cs.flags                &= IS_AT_THE_BEGINNING;
		/**/
		cs.do_loop_hook          = false;
		cs.do_follow_hook        = false;
		cs.do_loop_shoot         = false;
		cs.do_follow_shoot       = false;
		/**/
		cs.width        = 1;

		// Translate char
		switch (*s) {
			case '^': {
				compile_hologram(*s, &cs);
			} break;
			case '.': {
				compile_dot(&cs);
			} break;
			case '\\': {
				s += 1;
				assert((compile_escape(*s, &cs) || compile_hologram(*s, &cs)) && "Unknown escape.");
			} break;
			case '[': {
				s += compile_range(s, &cs) - 1;
			} break;
			default: { // Literal
				whitelist[0] =   *s;
				whitelist[1] = '\0';
			} break;
		}
		
		s += 1;

		// Compile blacklist
		if (*blacklist) {
			char filtered_blacklist[64];
			filtered_blacklist[0] = '\0'; 
			filter_blacklist(whitelist, blacklist, filtered_blacklist);
			HOOK_ALL(0, filtered_blacklist, HALT_AND_CATCH_FIRE, &cs);
		}

		if (cs.flags & DO_SKIP) {
			goto long_continue;
		}

		// Compile with quantifier
		switch (*s) {
			case '=':
			case '?': {
				HOOK_ALL(0, whitelist, +1, &cs);
				if ((cs.flags & DO_CATCH) || (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				s += 1;
			} break;
			case '*': {
				HOOK_ALL(0, whitelist,  0, &cs);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, 0, 1, 1, &cs);
				}
				s += 1;
			} break;
			case '+': {
				HOOK_ALL(0, whitelist, +1, &cs);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				++cs.state;
				HOOK_ALL(0, whitelist,  0, &cs);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, 0, 1, 1, &cs);
				}
				s += 1;
			} break;
			default: { // Literal
				HOOK_ALL(0, whitelist, +1, &cs);
				if ((cs.flags & DO_CATCH)
				||  (cs.flags & IS_NEGATIVE)) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				++cs.state;
			} break;
		}

		long_continue:
		cs.flags &= !IS_AT_THE_BEGINNING;
	}

	// Init state hookups
	ABSOLUTE_OFFSHOOT(0, 2, 0, 0, &cs);
	if (cs.flags & FORCE_START_OF_STRING) {
		ABSOLUTE_OFFSHOOT(1, HALT_AND_CATCH_FIRE, 0, 0, &cs);
	} else {
		ABSOLUTE_OFFSHOOT(1, 2, 0, 0, &cs);
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

	const char * s = string;
	LOOP: {
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
				const int r = regex_assert(regex, s + delta->width, delta->to, match);
				if(r){
					if ((match->position != -1)
					&&  (delta->match_width)) {
						match->position = (s - string);
					}
					match->width += delta->match_width;
					return r;
				}
			}
		}
	}

	PERFORM_CATCH_LOOKUP: {
		const offshoot_t * const my_catch = catch_table_lookup(regex, &state);
		if (my_catch && (!my_catch->width || !last_stand)) {
			state = my_catch->to;
			s += my_catch->width;
			match->width += my_catch->match_width;
			goto LOOP;
		}
	}

	return (state == regex->accepting_state);
}

match_t * regex_match(const regex_t * const              regex,
                      const char    * const             string,
                      const bool            is_start_of_string) {
	if (regex == NULL) {
		return NULL;
	}

	match_t * match = (match_t *)malloc(sizeof(match_t));

	if (string == NULL) {
		match->position = -1;
		match->width    =  0;
		return match;
	}

	const int initial_state = (int)(!is_start_of_string);

	// XXX: this should be called in a loop, always restarting from the last char of the last match
	if (regex_assert(regex, string, initial_state, match)) {
		return match;
	} else {
		return NULL;
	}
}

bool regex_search(const regex_t * const  regex,
                  const char    * const string) {

	return (bool)regex_match(regex, string, true);
}
