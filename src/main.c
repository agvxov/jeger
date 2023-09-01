// @COMPILECMD g++ $@ -o regtest -O0 -ggdb -pg -fno-inline
#include <stdio.h>
#include "regex.hpp"

#define TEST(a, b, expected) do { \
	r = regex_compile(a); \
	bool result = regex_search(r, b); \
	bool passed = (result == expected); \
	if (passed) { printf("Success.  - "); } else { printf("Failiour. - "); } \
	printf("%s vs %s: Result = %d, Expected = %d\n", #a, #b, result, expected); \
	++num_tests; \
	if (passed) { ++passed_tests; } \
} while(0)

signed main() {
	int num_tests = 0;
	int passed_tests = 0;
	regex_t * r;

	TEST(R"del(abc)del","abc",true);
	TEST(R"del(efg1)del","efg1",true);
	TEST(R"del(nig)del","ger",false);

	puts("");

	TEST(R"del(ab+c)del","abc",true);
	TEST(R"del(ef+g1)del","effffg1",true);
	TEST(R"del(ni*g?)del","ngg",false);

	puts("");

	TEST(R"del(ne.)del","net",true);
	TEST(R"del(ne.)del","ne",false);
	TEST(R"del(ne.+)del","neoo",true);

	puts("");

	TEST(R"del(ne.)del","ne\t",true);
	TEST(R"del(ne\t)del","ne",false);
	TEST(R"del(ne\t)del","ne\t",true);

	puts("");

	TEST(R"del(\sa)del"," a",true);
	TEST(R"del(\wi)del","hi",true);
	TEST(R"del(\w+)del","asd",true);

	puts("");

	TEST(R"del([A-Za-z]+)del","HelloWorld",true);
	TEST(R"del([A-Za-z]+g)del","HelloWorldg",true);
	TEST(R"del([A-Za-z]+g)del","g",false);

	puts("");

	TEST(R"del(a+a)del","aaa",true);
	TEST(R"del(a+a)del","aa",true);
	TEST(R"del(a+a)del","a",false);

	//++num_tests; TEST(R"del(\d{3})del","123",true);
	//++num_tests; TEST(R"del(^\w+@\w+\.\w+$)del","example@email.com",true);

	//++num_tests; TEST(R"del(\b\w+\b)del","This is a test",true);
	//++num_tests; TEST(R"del(^[A-Za-z]+\s\d+)del","OpenAI 123",true);
	//++num_tests; TEST(R"del([0-9]{4}-[0-9]{2}-[0-9]{2})del","2023-08-22",true);

	//++num_tests; TEST(R"del(^[^abc]+$)del","def123",true);
	//++num_tests; TEST(R"del(\b\d{5}\b)del","12345 67890",true);
	//++num_tests; TEST(R"del(^[A-Z][a-z]+$)del","OpenAI",true);

	//++num_tests; TEST(R"del(\d{3}-\d{2}-\d{4})del","123-45-6789",true);
	//++num_tests; TEST(R"del(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})del","192.168.1.1",true);
	//++num_tests; TEST(R"del(^\w{8,12})del","Password123", false);

	printf("\nPassed %d out of %d tests.\n", passed_tests, num_tests);
}
