#include "jeger.h"

static int test_counter       = 0;
static int passed_tests       = 0;
static int positives          = 0;
static int positive_successes = 0;
static int negatives          = 0;
static int negative_successes = 0;

static int test_counter2  = 0;
static int passed_tests2  = 0;

static
void asprint_match_t(      char    * *       destination,
                     const match_t *   const       match){
	if (match) {
		asprintf(destination, "%p {%d, %d}", (void *)match, match->position, match->width);
	} else {
		asprintf(destination, "0x000000000000 {N/A, N/A}");
	}
}

static
void print_leader(const bool passed){
	if (passed) {
		printf("\033[32;1mSuccess\033[0;1m.  - \033[0m");
	} else {
		printf("\033[31;1mFailiour\033[0;1m. - \033[0m");
	}
}

static
void do_flush(void) {
	if(!(test_counter % 5)) {
		fflush(stdout);
	}
}

static
void TEST(const char * const   what,
          const char * const     on,
	      const bool         expect){

	do_flush();

	regex_t * r = regex_compile(what);
	bool result = regex_search(r, on);
	regex_free(r);

	bool passed = (result == expect);

	expect ? ++positives : ++negatives;

	print_leader(passed);

	char * quoted_what, * quoted_on;
	asprintf(&quoted_what, "'%s'", what);
	asprintf(&quoted_on,   "'%s'", on);
	printf("%14s\033[1m vs \033[0m%14s\033[1m:\033[0m Result = %d, Expected = %d\n", quoted_what, quoted_on, result, expect);
	free(quoted_what);
	free(quoted_on);
	if (passed) {
		++passed_tests;
		expect ? ++positive_successes : ++negative_successes;
	}

	++test_counter;
}

static
void TEST2(const char    * const   what,
           const char    * const     on,
	       const match_t         expect){

	do_flush();

	regex_t * r      = regex_compile(what);
	match_t * result = regex_match(r, on, true);
	bool passed = (result->position == expect.position
	               && result->width == expect.width
	              );

	print_leader(passed);

	char * quoted_what, * quoted_on;
		asprintf(&quoted_what, "'%s'", what);
		asprintf(&quoted_on,   "'%s'", on);
	char * result_string, * expect_string;
		asprint_match_t(&result_string,  result);
		asprint_match_t(&expect_string, &expect);
	printf("%s\033[1m vs \033[0m%s\033[1m:\033[0m\n\tResult   = %s\n\tExpected = %s\n", quoted_what, quoted_on, result_string, expect_string);
	free(quoted_what);
	free(quoted_on);
	free(result_string);
	free(expect_string);
	free(result);

	if (passed) {
		++passed_tests2;
	}

	++test_counter2;
}
