#include <regex.h>

static
void TEST(const char * const   what,
          const char * const     on,
	      const bool         expect){

	regex_t * r = regcomp(&r, pattern, REG_EXTENDED | REG_NOSUB);
	int result = regexec(regex, input, 0, NULL, 0);
	regfree(r);

#if DEBUG
	assert((result != -1) && (expect == (bool)result));
#endif
}

signed main(){
	TEST( R"del(abc)del",  "abc", true);
	TEST(R"del(efg1)del", "efg1", true);
	TEST( R"del(nig)del",  "ger", false);
	TEST(  R"del(ss)del",  "sss", true);
	TEST( R"del(sss)del",   "ss", false);

	TEST( R"del(ab\+c)del",     "abc", true);
	TEST(R"del(ef\+g1)del", "effffg1", true);
	TEST(R"del(efg1\+)del",     "efg", false);
	TEST(R"del(efg1\+)del",    "efg1", true);
	TEST(R"del(efg1\+)del",   "efg11", true);

	TEST( R"del(a\+a)del", "aaa", true);
	TEST( R"del(a\+a)del",  "aa", true);
	TEST( R"del(a\+a)del",   "a", false);
	TEST( R"del(a\+a)del", "aaa", true);
	TEST(R"del(a\++)del", "aaa", false);

	TEST( R"del(ab*c)del",     "abc", true);
	TEST(R"del(ef*g1)del", "effffg1", true);
	TEST(R"del(efg1*)del",     "efg", true);
	TEST(R"del(efg1*)del",    "efg1", true);
	TEST(R"del(efg1*)del",   "efg11", true);

	TEST( R"del(ne.)del",  "net", true);
	TEST( R"del(ne.)del",   "ne", false);
	TEST(R"del(ne.\+)del", "neoo", true);
	TEST(R"del(ne.*)del", "neoo", true);
	TEST(R"del(ne.*)del",   "ne", true);

	TEST( R"del(ne.o)del",   "neto", true);
	TEST(R"del(ne.\+o)del", "nettto", true);
	TEST(R"del(ne.\+o)del",    "neo", false);
	TEST(R"del(ne.\+o)del",   "neoo", true);
	TEST(R"del(ne.*o)del",    "neo", true);

	TEST(R"del(ne.)del",  "ne\t", true);
	TEST(R"del(ne\t)del",   "ne", false);
	TEST(R"del(ne\t)del", "ne\t", true);
	TEST(R"del(ne\t)del",  "net", false);
	TEST(R"del(ne)del",   "ne\t", true);

	TEST(R"del(\sa)del",  " a", true);
	TEST(R"del(\sa)del", " a ", true);
	TEST(R"del(\wi)del",  "hi", true);
	TEST(R"del(\w\+)del", "asd", true);
	TEST(R"del(\w*)del",    "", true);

	TEST( R"del([A-Za-z]+)del", "HelloWorld",  true);
	TEST(R"del([A-Za-z]+g)del", "HelloWorldg", true);
	TEST(R"del([A-Za-z]+g)del",           "g", false);
	TEST(R"del([A-Za-z]*g)del",           "g", true);
	TEST(R"del([A-Za-z]+1)del",           "1", false);

	TEST(     R"del([^0-9])del",          "0", false);
	TEST(  R"del([^A-Za-z])del", "HelloWorld", false);
	TEST(R"del([^A-Za-z]+g)del",       "313g", true);
	TEST(     R"del([^0-9])del", "HelloWorld", true);
	TEST(       R"del([^a])del",         "ba", true);

	TEST( R"del(^\^)del",  "^^", true);
	TEST( R"del(^\^)del",  " ^", false);
	TEST(R"del(^ \^)del",  " ^", true);
	TEST( R"del(^a*)del", "asd", true);
	TEST(   R"del(^)del",    "", true);

	TEST(  R"del(\<test)del",  "test", true);
	TEST(  R"del(test\>)del",  "test", true);
	TEST(  R"del(\<test)del", "atest", false);
	TEST(  R"del(test\>)del", "testa", false);
	TEST(R"del(\<test\>)del",  "test", true);
}
