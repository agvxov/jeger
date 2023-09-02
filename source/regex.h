#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>

#include "vector.h"

extern bool is_case_on;

typedef struct {
	int accepting_state;
	char * str;
	vector_t delta_table;	// <delta_t>
	vector_t catch_table;	// <offshoot_t>
} regex_t;

typedef struct {
	int position;
	int width;
} match_t;

extern regex_t * regex_compile(const char * const pattern);
extern int       regex_free(regex_t * const regex);
extern bool      regex_search(const regex_t * const regex, const char * const string);
extern match_t * regex_match(const regex_t * const regex, const char * const string, const bool start_of_string);

extern bool is_magic(const char c);

#endif
