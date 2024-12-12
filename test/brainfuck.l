/* @BAKE
    #jeger --debug --trace -o $*.c $@
    jeger $@
    gcc  -o $* $*.c -ggdb
   @STOP
 */
%{
    #include <stdio.h>
    char data[30000];
    char * data_ptr = data;
%}

%x INITIAL IN_SKIP_FORWARD IN_SKIP_BACKWARD
%%

<INITIAL>{
\>   { ++data_ptr; }
\<   { --data_ptr; }
\+   { ++(*data_ptr); }
\-   { --(*data_ptr); }
\.   { putchar(*data_ptr); }
\,   { *data_ptr = getchar(); }
\[   {
        if (!*data_ptr) {
            BEGIN IN_SKIP_FORWARD;
        }
     }
\]   {
        if (!*data_ptr) {
            REVERSE;
            BEGIN IN_SKIP_BACKWARD;
        }
     }
}

<IN_SKIP_FORWARD>{
\]     { BEGIN INITIAL; }
}

<IN_SKIP_BACKWARD>{
\[     { REVERSE; BEGIN INITIAL; }
}

<INITIAL,IN_SKIP_FORWARD,IN_SKIP_BACKWARD>{
.|\n   { ; }
}
%%

signed main(int argc, char * argv[]) {
    if (argc != 2) {
        printf("%s <file>", argv[0]);
        return 1;
    }

    FILE * yyin = fopen(argv[1], "r");
    if (!yyin) { return 2; }

    fseek(yyin, 0, SEEK_END);
    int yylen = ftell(yyin);
    rewind(yyin);
    char yystr[yylen+1];
    yystr[yylen] = '\00';
    fread(yystr, yylen, sizeof(char), yyin);

    yylex(yystr);

    fclose(yyin);

    return 0;
}
