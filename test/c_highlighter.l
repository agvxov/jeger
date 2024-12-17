%{
    #include <stdio.h>

    int had_separation = 1;
%}
%x INITIAL IN_STRING IN_COMMENT IN_MULTILINE_COMMENT
%%

<INITIAL>{
inline  |
static  |
int     |
double  |
bool    |
float   |
long    |
short   |
char    {
            had_separation = 0;
            printf("\033[32m%.*s\033[0m", mlen, yytext);
        }
for     |
while   |
do      |
switch  {
            had_separation = 0;
            printf("\033[33m%.*s\033[0m", mlen, yytext);
        }
\/\/    {
            had_separation = 1;
            BEGIN IN_COMMENT;
        }
\/\*    {
            had_separation = 1;
            BEGIN IN_MULTILINE_COMMENT;
        }
[ \t\n\[\](){}+-*/%=] {
            had_separation = 1;
            putchar(yytext[0]);
        }
.       {
            had_separation = 0;
            putchar(yytext[0]);
        }
}

<IN_COMMENT>{
\n      {
            BEGIN INITIAL;
        }
.       { ; }
}

<IN_MULTILINE_COMMENT>{
\*\/    {
            BEGIN INITIAL;
        }
.|\n    { ; }
}
%%

#include "c_source_code_str.inc";

signed main(void) {
    yylex(source_code);

    return 0;
}
