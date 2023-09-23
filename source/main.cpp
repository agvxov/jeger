#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.hpp"

signed main() {
	TEST( R"del(abc)del",  "abc",  true);
	TEST(R"del(efg1)del", "efg1",  true);
	TEST( R"del(nig)del",  "ger", false);
	TEST(  R"del(ss)del",  "sss",  true);
	TEST( R"del(sss)del",   "ss", false);

	puts("");

	TEST( R"del(ab+c)del",     "abc",  true);
	TEST(R"del(ef+g1)del", "effffg1",  true);
	TEST(R"del(efg1+)del",     "efg", false);
	TEST(R"del(efg1+)del",    "efg1",  true);
	TEST(R"del(efg1+)del",   "efg11",  true);

	puts("");

	TEST( R"del(a+a)del", "aaa",  true);
	TEST( R"del(a+a)del",  "aa",  true);
	TEST( R"del(a+a)del",   "a", false);
	TEST( R"del(a+a)del", "aaa",  true);
	TEST(R"del(a+\+)del", "aaa", false);

	puts("");

	TEST( R"del(ab*c)del",     "abc", true);
	TEST(R"del(ef*g1)del", "effffg1", true);
	TEST(R"del(efg1*)del",     "efg", true);
	TEST(R"del(efg1*)del",    "efg1", true);
	TEST(R"del(efg1*)del",   "efg11", true);

	puts("");

	TEST( R"del(ne.)del",  "net",  true);
	TEST( R"del(ne.)del",   "ne", false);
	TEST(R"del(ne.+)del", "neoo",  true);
	TEST(R"del(ne.*)del", "neoo",  true);
	TEST(R"del(ne.*)del",   "ne",  true);

	puts("");

	TEST( R"del(ne.o)del",   "neto",  true);
	TEST(R"del(ne.+o)del", "nettto",  true);
	TEST(R"del(ne.+o)del",    "neo", false);
	TEST(R"del(ne.+o)del",   "neoo",  true);
	TEST(R"del(ne.*o)del",    "neo",  true);

	puts("");

	TEST(R"del(ne.)del",  "ne\t",  true);
	TEST(R"del(ne\t)del",   "ne", false);
	TEST(R"del(ne\t)del", "ne\t",  true);
	TEST(R"del(ne\t)del",  "net", false);
	TEST(R"del(ne)del",   "ne\t",  true);

	puts("");

	TEST(R"del(\sa)del",  " a", true);
	TEST(R"del(\sa)del", " a ", true);
	TEST(R"del(\wi)del",  "hi", true);
	TEST(R"del(\w+)del", "asd", true);
	TEST(R"del(\w*)del",    "", true);

	puts("");

	TEST( R"del([A-Za-z]+)del", "HelloWorld",   true);
	TEST(R"del([A-Za-z]+g)del", "HelloWorldg",  true);
	TEST(R"del([A-Za-z]+g)del",           "g", false);
	TEST(R"del([A-Za-z]*g)del",           "g",  true);
	TEST(R"del([A-Za-z]+1)del",           "1", false);

	puts("");

	TEST(     R"del([^0-9])del",          "0", false);
	TEST(  R"del([^A-Za-z])del", "HelloWorld", false);
	TEST(R"del([^A-Za-z]+g)del",       "313g",  true);
	TEST(     R"del([^0-9])del", "HelloWorld",  true);
	TEST(       R"del([^a])del",         "ba",  true);

	puts("");

	TEST( R"del(^\^)del",  "^^",  true);
	TEST( R"del(^\^)del",  " ^", false);
	TEST(R"del(^ \^)del",  " ^",  true);
	TEST( R"del(^a*)del", "asd",  true);
	TEST(   R"del(^)del",    "",  true);

	puts("");

	TEST(  R"del(\<test)del",  "test",  true);
	TEST(  R"del(test\>)del",  "test",  true);
	TEST(  R"del(\<test)del", "ttest", false);
	TEST(  R"del(test\>)del", "testa", false);
	TEST(R"del(\<test\>)del",  "test",  true);

	puts("");

	TEST(    R"del(\<int\>)del",  "printf", false);
	TEST(R"del(.\<print\>.)del", " print ",  true);
	TEST(R"del(.\<print\>.)del",  "fprint", false);
	TEST(R"del(.\<print\>.)del",  "printf", false);
	TEST(R"del(.\<print\>.)del", "fprintf", false);

	puts("");

	TEST(R"del(\<while\>)del",       "while", true);
	TEST(R"del(\<while\>)del",     " while ", true);
	TEST(R"del(\<while\>)del",     "9while ", true);
	TEST(R"del(\<while\>)del", "for while {", true);
	TEST(R"del(\<while\>)del",  "for while{", true);

	if (test_counter == passed_tests) {
		fputs("\033[32m", stdout);
	} else {
		fputs("\033[31m", stdout);
	}
	printf("\nPassed %d out of %d tests.\033[0m\n", passed_tests, test_counter);
	printf("\tPositives: %d/%d\n", positive_successes, positives);
	printf("\tNegatives: %d/%d\n", negative_successes, negatives);

	puts("");
	puts("");

	TEST2( R"del(abc)del",  "abc", match_t{ 0,  strlen("abc")});
	TEST2(R"del(efg1)del", "efg1", match_t{ 0, strlen("efg1")});
	TEST2( R"del(nig)del",  "ger", match_t{-1,             -1});
	TEST2(  R"del(ss)del",  "sss", match_t{ 0,              2});
	TEST2( R"del(sss)del",   "ss", match_t{-1,             -1});

	puts("");
	puts("");

	TEST2( R"del(ab+c)del",     "abc", match_t{ 0,     strlen("abc")});
	TEST2(R"del(ef+g1)del", "effffg1", match_t{ 0, strlen("effffg1")});
	TEST2(R"del(efg1+)del",     "efg", match_t{-1,                -1});
	TEST2(R"del(efg1+)del",    "efg1", match_t{ 0,    strlen("efg1")});
	TEST2(R"del(efg1+)del",   "efg11", match_t{ 0,   strlen("efg11")});

	puts("");
	puts("");

	TEST2( R"del(a+a)del",   " aaa", match_t{ 1,  strlen("aaa")});
	TEST2( R"del(a+a)del",    " aa", match_t{ 1,   strlen("aa")});
	TEST2( R"del(a+a)del",     " a", match_t{-1,             -1});
	TEST2( R"del(a+a)del", "   aaa", match_t{ 3,  strlen("aaa")});
	TEST2(R"del(a+\+)del",   "aaa+", match_t{ 0, strlen("aaa+")});

	puts("");
	puts("");

	TEST2(R"del(\<while\>)del",      "while", match_t{0, strlen("while")});
	TEST2(R"del(\<while\>)del",     " while", match_t{1, strlen("while")});
	TEST2(R"del(\<while\>)del",  "for while", match_t{4, strlen("while")});
	TEST2(R"del(\<while\>)del",  "for9while", match_t{4, strlen("while")});
	TEST2(R"del(\<while\>)del", "for9while ", match_t{4, strlen("while")});

	puts("");
	puts("");

	TEST2(R"del(\+)del", "akjh ab+ snabd", match_t{      strlen("akjh ab+")-1, 1});
	TEST2(R"del(\*)del", "a*jh abn snabd", match_t{            strlen("a*")-1, 1});
	TEST2(R"del(\=)del", "ak=h abn snabd", match_t{           strlen("ak=")-1, 1});
	TEST2(R"del(\?)del", "akjh abn s?abd", match_t{   strlen("akjh abn s?")-1, 1});
	TEST2(R"del(\+)del", "akjh abn snab+", match_t{strlen("akjh abn snab+")-1, 1});

	if(test_counter2 == passed_tests2) {
		fputs("\033[32m", stdout);
	} else {
		fputs("\033[31m", stdout);
	}
	printf("\nPassed %d out of %d tests.\033[0m\n", passed_tests2, test_counter2);

	return 0;
}
