#include <stdio.h>
#include "regex.h"

static int test_counter  = 0;
static int passed_tests = 0;
static int positives = 0;
static int positive_successes = 0;
static int negatives = 0;
static int negative_successes = 0;

static void
TEST(const char * const   what,
     const char * const     on,
	 const bool         expect){

	regex_t * r = regex_compile(what);
	bool result = regex_search(r, on);
	bool passed = (result == expect);

	expect ? ++positives : ++negatives;

	if (passed) {
		printf("\033[32;1mSuccess\033[0;1m.  - \033[0m");
		expect ? ++positive_successes : ++negative_successes;
	} else {
		printf("\033[31;1mFailiour\033[0;1m. - \033[0m");
	}
	printf("'%12s'\033[1m vs \033[0m'%12s'\033[1m:\033[0m Result = %d, Expected = %d\n", what, on, result, expect);
	if (passed) {
		++passed_tests;
	}

	++test_counter;
}

signed main() {
	TEST(R"del(abc)del","abc",true);
	TEST(R"del(efg1)del","efg1",true);
	TEST(R"del(nig)del","ger",false);
	TEST(R"del(ss)del","sss",true);
	TEST(R"del(sss)del","ss",false);

	puts("");

	TEST(R"del(ab+c)del","abc",true);
	TEST(R"del(ef+g1)del","effffg1",true);
	TEST(R"del(efg1+)del","efg",false);
	TEST(R"del(efg1+)del","efg1",true);
	TEST(R"del(efg1+)del","efg11",true);

	puts("");

	TEST(R"del(a+a)del","aaa",true);
	TEST(R"del(a+a)del","aa",true);
	TEST(R"del(a+a)del","a",false);
	TEST(R"del(a+a)del","aaa",true);
	TEST(R"del(a+\+)del","aaa",false);

	puts("");

	TEST(R"del(ab*c)del","abc",true);
	TEST(R"del(ef*g1)del","effffg1",true);
	TEST(R"del(efg1*)del","efg",true);
	TEST(R"del(efg1*)del","efg1",true);
	TEST(R"del(efg1*)del","efg11",true);

	puts("");

	TEST(R"del(ne.)del","net",true);
	TEST(R"del(ne.)del","ne",false);
	TEST(R"del(ne.+)del","neoo",true);
	TEST(R"del(ne.*)del","neoo",true);
	TEST(R"del(ne.*)del","ne",true);

	puts("");

	TEST(R"del(ne.)del","ne\t",true);
	TEST(R"del(ne\t)del","ne",false);
	TEST(R"del(ne\t)del","ne\t",true);
	TEST(R"del(ne\t)del","net",false);
	TEST(R"del(ne)del","ne\t",true);

	puts("");

	TEST(R"del(\sa)del"," a",true);
	TEST(R"del(\sa)del"," a ",true);
	TEST(R"del(\wi)del","hi",true);
	TEST(R"del(\w+)del","asd",true);
	TEST(R"del(\w*)del","",true);

	puts("");

	TEST(R"del([A-Za-z]+)del","HelloWorld",true);
	TEST(R"del([A-Za-z]+g)del","HelloWorldg",true);
	TEST(R"del([A-Za-z]+g)del","g",false);
	TEST(R"del([A-Za-z]*g)del","g",true);
	TEST(R"del([A-Za-z]+1)del","1",false);

	puts("");

	TEST(R"del(^\^)del","^^",true);
	TEST(R"del(^\^)del"," ^",false);
	TEST(R"del(^ \^)del"," ^",true);
	TEST(R"del(^a*)del","asd",true);
	TEST(R"del(^)del","",true);

	puts("");

	TEST(R"del(\<test)del","test",true);
	TEST(R"del(test\>)del","test",true);
	TEST(R"del(\<test)del","atest",false);
	TEST(R"del(test\>)del","testa",false);
	TEST(R"del(\<test\>)del","test",true);

	//TEST(R"del(\d{3})del","123",true);
	//TEST(R"del(^\w+@\w+\.\w+$)del","example@email.com",true);

	//TEST(R"del(\b\w+\b)del","This is a test",true);
	//TEST(R"del(^[A-Za-z]+\s\d+)del","OpenAI 123",true);
	//TEST(R"del([0-9]{4}-[0-9]{2}-[0-9]{2})del","2023-08-22",true);

	//TEST(R"del(^[^abc]+$)del","def123",true);
	//TEST(R"del(\b\d{5}\b)del","12345 67890",true);
	//TEST(R"del(^[A-Z][a-z]+$)del","OpenAI",true);

	//TEST(R"del(\d{3}-\d{2}-\d{4})del","123-45-6789",true);
	//TEST(R"del(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})del","192.168.1.1",true);
	//TEST(R"del(^\w{8,12})del","Password123", false);

	if(test_counter == passed_tests) {
		fputs("\033[32m", stdout);
	}
	printf("\nPassed %d out of %d tests.\033[0m\n", passed_tests, test_counter);
	printf("\tPositives: %d/%d\n", positive_successes, positives);
	printf("\tNegatives: %d/%d\n", negative_successes, negatives);
}
