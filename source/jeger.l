%{
    /* NOTE:
        its technically very bad taste to implement
        a minimalist lex subset with flex.
        maybe it should be reimplemented in pure C when possible.
     */
    #include "jeger.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdarg.h>
    #include <map>
    #include <vector>
    #include <string>

    using namespace std;

    void set_alphanet_range(char s, char e) {
        // XXX not implemented
    }

    void yyerror(const char * fmt, ...) {
        va_list va;
        va_start(va, fmt);

        fprintf(stderr, "jeger:error:%d: ", yylineno);
        vfprintf(stderr, fmt, va);
        fprintf(stderr, "\n");

        va_end(va);
    }

    typedef enum {
        STATIC_TABLE,
        SWITCH_TABLE,
    } table_t;

    typedef struct {
        char * pattern;
        char * code;
    } rule_t2;

    string definition_section_code_buffer_str;
    string code_section_code_buffer_str;

    map<string, vector<rule_t2>> rules;
    map<string, vector<rule_t2>>::iterator current_state;
    string patter_buffer;
    string code_buffer;

    char * prefix = strdup("yy");
    table_t table_type = STATIC_TABLE;
    bool do_setup_lineno = false;

    int nest_counter = 0;
%}
%x IN_DEFINITION_SECTION IN_RULE_SECTION IN_CODE_SECTION
%x IN_DEFINITION_SECTION_CODE
%x IN_RULE_LIST IN_OPTION_LIST
%x IN_STATE_DEFINITION
%x IN_CODE IN_STRING IN_COMMENT IN_MULTILINE_COMMENT

rule_name [A-Z_][A-Z0-9_]*
ws        [ \t\r\v\f]
wsnl      [ \t\r\v\f\n]
value     \"[-a-z]+\"

%option yylineno
%option nodefault
%option noyywrap
%%
    BEGIN IN_DEFINITION_SECTION;

<IN_DEFINITION_SECTION>{
\%\%            {
                    BEGIN IN_RULE_SECTION;
                }
^\%\{           {
                    if (definition_section_code_buffer_str != "") {
                        return 1;
                    }

                    BEGIN IN_DEFINITION_SECTION_CODE;
                }
\%x             {
                    BEGIN IN_RULE_LIST;
                }
\%option        {
                    BEGIN IN_OPTION_LIST;
                }
.               {
                    yyerror("baaaa");
                }
\n              { ; }
}

<IN_RULE_LIST>{
{rule_name}     {
                    rules[yytext] = {};
                }
{ws}*           { ; }
\n              {
                    BEGIN IN_DEFINITION_SECTION;
                }
}

<IN_OPTION_LIST>{
yylineno            {
                        do_setup_lineno = true;
                    }
7bit                {
                        set_alphanet_range((char)0, (char)127);
                    }
8bit                {
                        set_alphanet_range((char)0, (char)255);
                    }
alphabet={value}    {
                        // XXX
                    }
table={value}       {
                        // NOTE: this might be bad taste
                        const char * value = yytext + strlen("table=");
                        int value_len = yyleng - (strlen("table=") + 2 /* "" */);

                        if (!strncmp(value, "switch", value_len)) {
                            table_type = SWITCH_TABLE;
                        } else
                        if (!strncmp(value, "static", value_len)) {
                            table_type = STATIC_TABLE;
                        }
                    }
prefix={value}      {
                        free(prefix);
                        prefix = strdup(yytext);
                    }
{ws}*               { ; }
[^ \t\n]+           {
                        yyerror("Unknown string pretending to be an option ('%s').", yytext);
                    }
\n                  {
                        BEGIN IN_DEFINITION_SECTION;
                    }
}

<IN_DEFINITION_SECTION_CODE>{
.|\n    {
            definition_section_code_buffer_str += yytext;
        }
^\%\}   {
            BEGIN IN_DEFINITION_SECTION;
        }
}

<IN_RULE_SECTION>{
\%\%                {
                        BEGIN IN_CODE_SECTION;
                    }
\<{rule_name}\>\{   {
                        string state_name(yytext+1, yyleng-3);
                        current_state = rules.find(state_name);
                        if (current_state == rules.end()) {
                            yyerror("State '%s' was never declared.", state_name.c_str());
                        }

                        patter_buffer = "";
                        code_buffer = "";

                        BEGIN IN_STATE_DEFINITION;
                    }
.                   {
                        yyerror("baaa");
                    }
\n                  { ; }
}

<IN_STATE_DEFINITION>{
.           {
                patter_buffer += yytext;
            }
{wsnl}+\{   {
                BEGIN IN_CODE;
                nest_counter = 0;
            }
\n          { ; }
}

<IN_CODE>{
\{      {
            ++nest_counter;
        }
\}      {
            --nest_counter;
            if (nest_counter == -1) {
                current_state->second.push_back((rule_t2) {
                    .pattern = strdup(patter_buffer.c_str()),
                    .code    = strdup(code_buffer.c_str()),
                });

                BEGIN IN_RULE_SECTION;
            }
        }
\"      {
            BEGIN IN_STRING;
        }
\/\/    {
            BEGIN IN_COMMENT;
        }
\/\*    {
            BEGIN IN_MULTILINE_COMMENT;
        }
.|\n    {
            code_buffer += yytext;
        }
}

<IN_STRING>{
\\\\    { ; }
\\\"    { ; }
\"      { BEGIN IN_CODE; }
.|\n    { ; } /* XXX we are eating strings */
}

<IN_COMMENT>{
.       { ; }
\n      { BEGIN IN_CODE; }
}

<IN_MULTILINE_COMMENT>{
.|\n    { ; }
\*\/    { BEGIN IN_CODE; }
}

<IN_CODE_SECTION>{
(.|\n)* {
            code_section_code_buffer_str += yytext;
        }
}

%%

static
void dump_parse_results(void) {
    puts(definition_section_code_buffer_str.c_str());
    puts("----------");

    for (const auto &i : rules) {
        printf("%s:\n", i.first.c_str());
        for (const auto &h : i.second) {
            printf("\tpattern:\n%s\n" "\tcode:\n%s\n", h.pattern, h.code);
        }
        puts("--");
    }

    puts("----------");
    puts(code_section_code_buffer_str.c_str());
}

extern "C"
int parse(const char * filename) {
    int r = 0;
    FILE * f = fopen(filename, "r");
    if (!f) { return 2; }

    yyin = f;

    r = yylex();
    if (r) { return r; }

    dump_parse_results();

    patterns = (rule_t*)malloc(sizeof(rule_t)*(rules.size()+1));

    int i = 0;
    for (const auto &rule_it : rules) {
        for (const auto &rule : rule_it.second) {
            patterns[i++] = (rule_t) {
                .state = i,
                .pattern = rule.pattern,
                .code    = rule.code,
            };
        }
    }
    patterns[rules.size()] = (rule_t) { 0, NULL, NULL };

    definition_section_code_buffer = strdup(definition_section_code_buffer_str.c_str());
    code_section_code_buffer       = strdup(code_section_code_buffer_str.c_str());

    return r;
}
