#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "regex.h"

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
		asprintf(destination, "0x0 {N/A, N/A}");
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
void TEST(const char * const   what,
          const char * const     on,
	      const bool         expect){

	regex_t * r = regex_compile(what);
	bool result = regex_search(r, on);
	bool passed = (result == expect);

	passed && expect ? ++positive_successes : ++negative_successes;

	print_leader(passed);

	char * quoted_what, * quoted_on;
	asprintf(&quoted_what, "'%s'", what);
	asprintf(&quoted_on,   "'%s'", on);
	printf("%14s\033[1m vs \033[0m%14s\033[1m:\033[0m Result = %d, Expected = %d\n", quoted_what, quoted_on, result, expect);
	free(quoted_what);
	free(quoted_on);
	if (passed) {
		++passed_tests;
	}

	++test_counter;
}

static
void TEST2(const char    * const   what,
           const char    * const     on,
	       const match_t         expect){

	regex_t * r      = regex_compile(what);
	match_t * result = regex_match(r, on, true);
	bool passed = (
	                (  result
	                 && result->position == expect.position
	                 && result->width    == expect.width
	                )
				  ||
				    expect.position == -1
	              );

	print_leader(passed);

	char * quoted_what, * quoted_on;
		asprintf(&quoted_what, "'%s'", what);
		asprintf(&quoted_on,   "'%s'", on);
	char * result_string, * expect_string;
		asprint_match_t(&result_string,  result);
		asprint_match_t(&expect_string, &expect);
	printf("%14s\033[1m vs \033[0m%14s\033[1m:\033[0m\n\t%s\n\t%s\n", quoted_what, quoted_on, result_string, expect_string);
	free(quoted_what);
	free(quoted_on);
	free(result_string);
	free(expect_string);
	if (passed) {
		++passed_tests2;
	}

	++test_counter2;
}

signed main() {
	TEST( R"del(abc)del",  "abc", true);
	TEST(R"del(efg1)del", "efg1", true);
	TEST( R"del(nig)del",  "ger", false);
	TEST(  R"del(ss)del",  "sss", true);
	TEST( R"del(sss)del",   "ss", false);

	puts("");

	TEST( R"del(ab+c)del",     "abc", true);
	TEST(R"del(ef+g1)del", "effffg1", true);
	TEST(R"del(efg1+)del",     "efg", false);
	TEST(R"del(efg1+)del",    "efg1", true);
	TEST(R"del(efg1+)del",   "efg11", true);

	puts("");

	TEST( R"del(a+a)del", "aaa", true);
	TEST( R"del(a+a)del",  "aa", true);
	TEST( R"del(a+a)del",   "a", false);
	TEST( R"del(a+a)del", "aaa", true);
	TEST(R"del(a+\+)del", "aaa", false);

	puts("");

	TEST( R"del(ab*c)del",     "abc", true);
	TEST(R"del(ef*g1)del", "effffg1", true);
	TEST(R"del(efg1*)del",     "efg", true);
	TEST(R"del(efg1*)del",    "efg1", true);
	TEST(R"del(efg1*)del",   "efg11", true);

	puts("");

	TEST( R"del(ne.)del",  "net", true);
	TEST( R"del(ne.)del",   "ne", false);
	TEST(R"del(ne.+)del", "neoo", true);
	TEST(R"del(ne.*)del", "neoo", true);
	TEST(R"del(ne.*)del",   "ne", true);

	puts("");

	TEST( R"del(ne.o)del",   "neto", true);
	TEST(R"del(ne.+o)del", "nettto", true);
	TEST(R"del(ne.+o)del",    "neo", false);
	TEST(R"del(ne.+o)del",   "neoo", true);
	TEST(R"del(ne.*o)del",    "neo", true);

	puts("");

	TEST(R"del(ne.)del",  "ne\t", true);
	TEST(R"del(ne\t)del",   "ne", false);
	TEST(R"del(ne\t)del", "ne\t", true);
	TEST(R"del(ne\t)del",  "net", false);
	TEST(R"del(ne)del",   "ne\t", true);

	puts("");

	TEST(R"del(\sa)del",  " a", true);
	TEST(R"del(\sa)del", " a ", true);
	TEST(R"del(\wi)del",  "hi", true);
	TEST(R"del(\w+)del", "asd", true);
	TEST(R"del(\w*)del",    "", true);

	puts("");

	TEST( R"del([A-Za-z]+)del", "HelloWorld",  true);
	TEST(R"del([A-Za-z]+g)del", "HelloWorldg", true);
	TEST(R"del([A-Za-z]+g)del",           "g", false);
	TEST(R"del([A-Za-z]*g)del",           "g", true);
	TEST(R"del([A-Za-z]+1)del",           "1", false);

	puts("");

	TEST(     R"del([^0-9])del",          "0", false);
	TEST(  R"del([^A-Za-z])del", "HelloWorld", false);
	TEST(R"del([^A-Za-z]+g)del",       "313g", true);
	TEST(     R"del([^0-9])del", "HelloWorld", true);
	TEST(       R"del([^a])del",         "ba", true);

	puts("");

	TEST( R"del(^\^)del",  "^^", true);
	TEST( R"del(^\^)del",  " ^", false);
	TEST(R"del(^ \^)del",  " ^", true);
	TEST( R"del(^a*)del", "asd", true);
	TEST(   R"del(^)del",    "", true);

	puts("");

	TEST(  R"del(\<test)del",  "test", true);
	TEST(  R"del(test\>)del",  "test", true);
	TEST(  R"del(\<test)del", "atest", false);
	TEST(  R"del(test\>)del", "testa", false);
	TEST(R"del(\<test\>)del",  "test", true);

	if(test_counter == passed_tests) {
		fputs("\033[32m", stdout);
	}
	printf("\nPassed %d out of %d tests.\033[0m\n", passed_tests, test_counter);
	printf("\tPositives: %d/%d\n", positive_successes, positives);
	printf("\tNegatives: %d/%d\n", negative_successes, negatives);

	puts("");

	puts("");

	TEST2( R"del(abc)del",  "abc", match_t{ 0, 3});
	TEST2(R"del(efg1)del", "efg1", match_t{ 0, 4});
	TEST2( R"del(nig)del",  "ger", match_t{-1, 0});
	TEST2(  R"del(ss)del",  "sss", match_t{ 0, 2});
	TEST2( R"del(sss)del",   "ss", match_t{-1, 0});
}
