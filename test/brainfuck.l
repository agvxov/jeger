/* @BAKE
    jeger --debug --trace -o $*.c $@
    gcc  -o $* $*.c -ggdb
   @STOP
 */
%{
    char data[30000];
    char * data_ptr = data;
%}

%x IN_SKIP_FORWARD IN_SKIP_BACKWARD

%option noyywrap nodefault
%%
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
.|\n { ; }

<IN_SKIP_FORWARD>{
\]     { BEGIN INITIAL; }
}

<IN_SKIP_BACKWARD>{
\[     { REVERSE; BEGIN INITIAL; }
}

<IN_SKIP_FORWARD,IN_SKIP_BACKWARD>{
.|\n   { ; }
}
%%

signed main(int argc, char * argv[]) {
    if (argc != 2) {
        printf("%s <file>", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        return 2;
    }

    yylex();
    yylex_destroy();

    return 0;
}
