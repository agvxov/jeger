/* regex.c
 * Copyright 2023 Anon Anonson, Ognjen 'xolatile' Milan Robovic, Emil Williams
 * SPDX Identifier: GPL-3.0-only / NO WARRANTY / NO GUARANTEE */

/* XXX:
 *  as it turns out returning a range of match objects is a
 *  high profile performance issue regarding regex, especially when highlighting.
 *  now as it stands we search an array of tokens for every position on a string.
 *  which sounds ok, until one realizes that searching from any position revails a range,
 *  where (future) matches can or cannot be found. meaning we are computing the same thing
 *  repeatedly, practically resulting in a bruteforcing situation where instead of eliminating
 *  certain non-matches, we blindly hammer character by character.
 */

/* XXX:
 *  the bigass char sets should be global and broken into parts
*/

#include "regex.h"

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

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

typedef struct {
	// XXX:
	//  These should share a mask
	//  Not even sure why they are pointers to begin with
	bool    * do_catch;
	bool    * is_negative;
	bool      is_at_the_beginning;
	bool      do_skip;
// these might be obsolite but im leaving them for now
	bool    * do_loop_hook;
	bool    * do_follow_hook;
	bool    * do_loop_shoot;
	bool    * do_follow_shoot;
// ---
	int     * state;
	int     * width;
	char    * whitelist;
	char    * blacklist;
	regex_t * regex;
} compiler_state;



// ----------------------------------
// ### Regex creation/destruction ###
// ----------------------------------
#define HALT_AND_CATCH_FIRE INT_MIN

static void HOOK_ALL(      int              from,
                     const char * const      str,
                           int                to,
                           compiler_state *   cs) {

	int hook_to = (to == HALT_AND_CATCH_FIRE) ? HALT_AND_CATCH_FIRE : ((*cs->state) + to);


	for (const char * s = str; *s != '\0'; s++) {
		delta_t * delta = malloc(sizeof(delta_t));
		delta->in    = *cs->state + from;
		delta->input = *s;
		delta->to    = hook_to;
		delta->width = *cs->width;
		vector_push(&cs->regex->delta_table,
		            &delta);
	}
}

// XXX: align
static void ABSOLUTE_OFFSHOOT(int             from,
                              int               to,
                              int            width,
                              int      match_width,
                              compiler_state *  cs) {
	offshoot_t * offshoot = malloc(sizeof(offshoot_t));
	offshoot->in    = from; 
	offshoot->to    = to;
	offshoot->width = width;
	offshoot->match_width = match_width;
	vector_push(&cs->regex->catch_table,
	            &offshoot);
}

// XXX: align
static void OFFSHOOT(int             from,
                     int               to,
                     int            width,
                            int      match_width,
                     compiler_state *  cs) {
	ABSOLUTE_OFFSHOOT(*cs->state + from, *cs->state + to, width, match_width, cs);
}

static int escape_1_to_1(const char c, compiler_state * cs) {
	char * target_list = (*cs->is_negative) ? cs->blacklist : cs->whitelist;
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

static int escape_1_to_N(const char c, compiler_state * cs) {
	char * target_list = (*cs->is_negative) ? cs->blacklist : cs->whitelist;
	switch(c) {
		case 'i': {
			const char identifier_chars[] = "@0123456789_"
			                                "\300\301\302\303\304"
			                                "\305\306\307\310\311"
			                                "\312\313\314\315\316"
			                                "\317\320\321\322\323"
			                                "\324\325\326\327\330"
			                                "\331\332\333\334\335"
			                                "\336\337";
			strcpy(target_list, identifier_chars);
			return sizeof(identifier_chars)-1;
		};
		case 'I': {
			const char identifier_chars[] = "@_"
			                                "\300\301\302\303\304"
			                                "\305\306\307\310\311"
			                                "\312\313\314\315\316"
			                                "\317\320\321\322\323"
			                                "\324\325\326\327\330"
			                                "\331\332\333\334\335"
			                                "\336\337";
			strcpy(target_list, identifier_chars);
			return sizeof(identifier_chars)-1;
		};
		case 'k': {
			const char keyword_chars[] = "@0123456789_"
			                             "\300\301\302\303\304"
			                             "\305\306\307\310\311"
			                             "\312\313\314\315\316"
			                             "\317\320\321\322\323"
			                             "\324\325\326\327\330"
			                             "\331\332\333\334\335"
			                             "\336\337";
			strcpy(target_list, keyword_chars);
			return sizeof(keyword_chars)-1;
		};
		case 'K': {
			const char keyword_chars[] = "@_"
			                             "\300\301\302\303\304"
			                             "\305\306\307\310\311"
			                             "\312\313\314\315\316"
			                             "\317\320\321\322\323"
			                             "\324\325\326\327\330"
			                             "\331\332\333\334\335"
			                             "\336\337";
			strcpy(target_list, keyword_chars);
			return sizeof(keyword_chars)-1;
		};
		case 'f': {
			const char filename_chars[] = "@0123456789/.-_+,#$%~=";
			strcpy(target_list, filename_chars);
			return sizeof(filename_chars)-1;
		};
		case 'F': {
			const char filename_chars[] = "@/.-_+,#$%~=";
			strcpy(target_list, filename_chars);
			return sizeof(filename_chars)-1;
		};
		case 'p': {
			const char printable_chars[] = "@"
			                               "\241\242\243\244\245"
			                               "\246\247\250\251\252"
			                               "\253\254\255\256\257"
			                               "\260\261\262\263\264"
			                               "\265\266\267\270\271"
			                               "\272\273\274\275\276"
			                               "\277"
			                               "\300\301\302\303\304"
			                               "\305\306\307\310\311"
			                               "\312\313\314\315\316"
			                               "\317\320\321\322\323"
			                               "\324\325\326\327\330"
			                               "\331\332\333\334\335"
			                               "\336\337";
			strcpy(target_list, printable_chars);
			return sizeof(printable_chars)-1;
		};
		case 'P': {
			const char printable_chars[] = "@"
			                               "\241\242\243\244\245"
			                               "\246\247\250\251\252"
			                               "\253\254\255\256\257"
			                               "\260\261\262\263\264"
			                               "\265\266\267\270\271"
			                               "\272\273\274\275\276"
			                               "\277"
			                               "\300\301\302\303\304"
			                               "\305\306\307\310\311"
			                               "\312\313\314\315\316"
			                               "\317\320\321\322\323"
			                               "\324\325\326\327\330"
			                               "\331\332\333\334\335"
			                               "\336\337";
			strcpy(target_list, printable_chars);
			return sizeof(printable_chars)-1;
		};
		case 's': {
			const char whitespace_chars[] = " \t\v\n";
			strcpy(target_list, whitespace_chars);
			return sizeof(whitespace_chars)-1;
		};
		case 'd': {
			const char digit_chars[] = "0123456789";
			strcpy(target_list, digit_chars);
			return sizeof(digit_chars)-1;
		};
		case 'x': {
			const char hex_chars[] = "0123456789"
			                         "abcdef"
			                         "ABCDEF";
			strcpy(target_list, hex_chars);
			return sizeof(hex_chars)-1;
		};
		case 'o': {
			const char oct_chars[] = "01234567";
			strcpy(target_list, oct_chars);
			return sizeof(oct_chars)-1;
		};
		case 'w': {
			const char word_chars[] = "0123456789"
			                          "abcdefghijklmnopqrstuwxyz"
			                          "ABCDEFGHIJKLMNOPQRSTUWXYZ"
			                          "_";
			strcpy(target_list, word_chars);
			return sizeof(word_chars)-1;
		};
		case 'h': {
			const char very_word_chars[] = "abcdefghijklmnopqrstuwxyz"
			                               "ABCDEFGHIJKLMNOPQRSTUWXYZ"
			                               "_";
			strcpy(target_list, very_word_chars);
			return sizeof(very_word_chars)-1;
		};
		case 'a': {
			const char alpha_chars[] = "abcdefghijklmnopqrstuwxyz"
			                           "ABCDEFGHIJKLMNOPQRSTUWXYZ";
			strcpy(target_list, alpha_chars);
			return sizeof(alpha_chars)-1;
		};
		case 'l': {
			const char lower_alpha_chars[] = "abcdefghijklmnopqrstuwxyz";
			strcpy(target_list, lower_alpha_chars);
			return sizeof(lower_alpha_chars)-1;
		};
		case 'u': {
			const char upper_alpha_chars[] = "ABCDEFGHIJKLMNOPQRSTUWXYZ";
			strcpy(target_list, upper_alpha_chars);
			return sizeof(upper_alpha_chars)-1;
		};
	}

	return 0;
}

static int escape_to_negative(const char              c,
	                                compiler_state * cs) {
	switch (c) {
		case 'D': {
			const char digit_chars[] = "0123456789";
			strcpy(cs->blacklist, digit_chars);
			*cs->is_negative = true;
			return sizeof(digit_chars)-1;
		};
	}

	return 0;
}

static int escape_hologram(const char c, compiler_state * cs) {
	switch (c) {
		case '<': {
			if (cs->is_at_the_beginning) {
				ABSOLUTE_OFFSHOOT(0, 2, 0, 0, cs);
				cs->do_skip = true;
			}
			const char very_word_chars[] = "abcw";
			//const char very_word_chars[] = "abcdefghijklmnopqrstuwxyz"
			//							   "ABCDEFGHIJKLMNOPQRSTUWXYZ"
			//							   "_";
			*cs->is_negative = true;	// effectless currently; should be used to trigger the following lines in the main compile loop
			strcat(cs->blacklist, very_word_chars);
			HOOK_ALL(0, cs->blacklist, HALT_AND_CATCH_FIRE, cs);
			OFFSHOOT(0, 0, 1, 0, cs);

			return sizeof(very_word_chars)-1;
		};
		case '>': {
			const char very_word_chars[] = "abcw";
			//const char very_word_chars[] = "abcdefghijklmnopqrstuwxyz"
			//							   "ABCDEFGHIJKLMNOPQRSTUWXYZ"
			//							   "_";
			*cs->is_negative = true;
			strcat(cs->blacklist, very_word_chars);
			
			return sizeof(very_word_chars)-1;
		}
	}
	return 0;
}

static int compile_dot(compiler_state * cs) {
	*cs->do_catch = true;
	return true;
}

static int compile_escape(const char                    c,
                                compiler_state *       cs) {

	return escape_1_to_1(c,      cs)
		|| escape_1_to_N(c,      cs)
		|| escape_to_negative(c, cs)
		|| escape_hologram(c,    cs)
		;
}

static int compile_range(const char           * const range,
                               compiler_state *          cs) {
	assert((range[0] == '[') && "Not a range.");

	const char * s;
	if (range[1] == '^') {
		*cs->is_negative = true;
		s = range + 2;
	} else {
		s = range + 1;
	}

	char * target_list = (*cs->is_negative) ? cs->blacklist : cs->whitelist;

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

	int state = 2;

	// this is plain retarded
	bool do_catch;
	bool is_negative;
	bool do_loop_hook;
	bool do_follow_hook;
	bool do_loop_shoot;
	bool do_follow_shoot;
	int width;
	char whitelist[64];
	char blacklist[64];

	compiler_state cs = {
		.do_catch            = &do_catch,
		.is_negative         = &is_negative,
		.is_at_the_beginning = true,
		.do_skip             = false,
		.state               = &state,
		.width               = &width,
		.whitelist           = whitelist,
		.blacklist           = blacklist,
		.regex               = regex,
	};

	for (const char * s = pattern; *s != '\00';) {
		// Reset the compiler
		assert(!is_quantifier(*pattern) && "Pattern starts with quantifier.");
		whitelist[0]    =  '\0';
		blacklist[0]    =  '\0';
		do_catch        = false;
		is_negative     = false;
		cs.do_skip      = false;
		/**/
		do_loop_hook    = false;
		do_follow_hook  = false;
		do_loop_shoot   = false;
		do_follow_shoot = false;
		/**/
		width        = 1;

		// Translate char
		switch (*s) {
			case '^': {
				if (cs.is_at_the_beginning) {
					ABSOLUTE_OFFSHOOT(0,                   2, 0, 0, &cs);
					ABSOLUTE_OFFSHOOT(1, HALT_AND_CATCH_FIRE, 0, 0, &cs);
				}
				whitelist[0] = '\n';
				whitelist[1] = '\0';
				HOOK_ALL(0, whitelist, 0, &cs);
				if (s != pattern) {
					state += 1;
				}
				cs.do_skip = true;
			} break;
			case '.': {
				compile_dot(&cs);
			} break;
			case '\\': {
				s += 1;
				assert(compile_escape(*s, &cs) && "Unknown escape.");
			} break;
			case '[': {
				s += compile_range(s, &cs) - 1;
			} break;
			default: {
				whitelist[0] =   *s;
				whitelist[1] = '\0';
			} break;
		}
		
		s += 1;

		if (cs.do_skip) {
			goto long_continue;
		}

		// Compile blacklist
		if (*blacklist) {
			char filtered_blacklist[64];
			filtered_blacklist[0] = '\0'; 
			filter_blacklist(whitelist, blacklist, filtered_blacklist);
			HOOK_ALL(0, filtered_blacklist, HALT_AND_CATCH_FIRE, &cs);
		}

		// Compile with quantifier
		switch (*s) {
			case '=':
			case '?': {
				do_loop_hook = true;
				HOOK_ALL(0, whitelist, +1, &cs);
				if (do_catch || is_negative) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				s += 1;
			} break;
			case '*': {
				HOOK_ALL(0, whitelist,  0, &cs);
				if (do_catch) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				} else if (is_negative) {
					OFFSHOOT(0,  0, 1, 1, &cs);
				}
				s += 1;
			} break;
			case '+': {
				HOOK_ALL(0, whitelist, +1, &cs);
				if (do_catch || is_negative) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				state += 1;
				HOOK_ALL(0, whitelist,  0, &cs);
				if (do_catch || is_negative) {
					OFFSHOOT(0, 0, 1, 1, &cs);
				}
				s += 1;
			} break;
			default: { // Literal
				HOOK_ALL(0, whitelist, +1, &cs);
				if (do_catch || is_negative) {
					OFFSHOOT(0, +1, 1, 1, &cs);
				}
				state += 1;
			} break;
		}

		long_continue:
		cs.is_at_the_beginning = false;
	}

	regex->accepting_state = state;

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
// XXX: rename; align
static offshoot_t * catch_(const regex_t * const regex,
                         int     * const state) {
	for (size_t i = 0; i < regex->catch_table.element_count; i++){
		const offshoot_t * const offshoot = *(offshoot_t**)vector_get(&regex->catch_table, i);
		if (offshoot->in == *state) {
			*state = offshoot->to;
			return offshoot;
		}
	}
	return NULL;
}

static bool regex_assert(const regex_t * const         regex,
                         const char    * const        string,
                         const int             string_offset,   // XXX: useless
                               int                     state,
                               match_t * const         match) {
	if (state == HALT_AND_CATCH_FIRE) { return false; }
	for (const char * s = (string + string_offset); *s != '\00';) {
		// XXX: this should be a jump search for the instate and then a linear
		for (size_t i = 0; i < regex->delta_table.element_count; i++) {
			const delta_t * const delta = *(delta_t**)vector_get(&regex->delta_table, i);
			if ((delta->in == state) 
			&&  (delta->input == *s)) {
				const int r = regex_assert(regex, string, (s - string) + delta->width, delta->to, match);
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

		const offshoot_t * const catch = catch_(regex, &state);
		if ((catch)
		&&  (state != HALT_AND_CATCH_FIRE)) {
			s += catch->width;
			match->width += catch->match_width;
			continue;
		} else {
			break;
		}
	}

	return (state == regex->accepting_state);
}

match_t * regex_match(      regex_t *                    regex,
                      const char    * const             string,
                      const bool            is_start_of_string,
                      const int                  string_offset) {	// XXX: remove this useless piece of shit of a parameter nigger
	if (regex == NULL) {
		return NULL;
	}

	match_t * m = (match_t *)malloc(sizeof(match_t));
	if (string == NULL) {
		m->position = -1;
		m->width    =  0;
		return m;
	}

	const int initial_state = (int)(!is_start_of_string);

	// XXX: this should be called in a loop, always restarting from the last char of the last match
	if(regex_assert(regex, string, string_offset, initial_state, m)) {
		return m;
	} else {
		free(m);
		return NULL;
	}
}

bool regex_search(      regex_t *        regex,
                  const char    * const string) {

	return (bool)regex_match(regex, string, true, 0);
}
