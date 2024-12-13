const char * yy_lookup_str = "\n\
static inline\n\
int mlookup(const char * s, int state) {\n\
    for (int i = 0; s[i] != '\\0'; i++) {\n\
        state = table[state][AS_SYMBOL(s[i])];\n\
        if (state == NO_MATCH) {\n\
            break;\n\
        } else\n\
        if (state > NO_MATCH) {\n\
            mlen = i+1;\n\
            return state;\n\
        }\n\
    }\n\
\n\
    mlen = 0;\n\
    return NO_MATCH;\n\
}\n"
;

const char * yy_lex_str_start = "\n\
int yylex(const char * s) {\n\
    int state = 0;\n\
    for (\n\
      const char * ss = s;\n\
      *ss != '\\0';\n\
      ss += ((mlen ? mlen : 1) * direction)\n\
    ) {\n\
        int match = mlookup(ss, state);\n\
        switch (match) {\n\
";

const char * yy_lex_str_end = "\n\
        }\n\
    }\n\
    return 0;\n\
}\n\
";
