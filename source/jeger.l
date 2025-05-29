%{
    /* NOTE:
        its technically very bad taste to implement
        a minimalist lex subset with flex.
        it was a devtime optimization.
        maybe it should be reimplemented in pure C when possible.
     */

  extern "C" {
    #include "dictate.h"
    #include "jeger.h"
    #include "opts.h"
  }

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdarg.h>
    #include <map>
    #include <vector>
    #include <string>

    char * prefix = strdup("yy");
    table_t table_type = STATIC_TABLE;
    bool do_setup_lineno = false;

    using namespace std;

    static void set_alphanet_range(char s, char e) {
        // XXX not implemented
    }

    static void yyerror(const char * fmt, ...) {
        va_list va;
        va_start(va, fmt);

        fprintf(stderr, "jeger:error:%d: ", yylineno);
        vfprintf(stderr, fmt, va);
        fprintf(stderr, "\n");

        va_end(va);
    }

    static int nest_counter = 0;
    static int source_state;
    static string * source_buffer;

    static string definition_section_code_buffer_str;
    static string code_section_code_buffer_str;

    static map<string, vector<rule_t>> rules_map;
    static vector<map<string, vector<rule_t>>::iterator> current_states;
    static vector<code_t> codes_vec;
    static string patter_buffer;
    static string code_buffer;
    static int line_buffer;
%}
%x IN_DEFINITION_SECTION IN_RULE_SECTION IN_CODE_SECTION
%x IN_DEFINITION_SECTION_CODE
%x IN_RULE_LIST IN_OPTION_LIST
%x IN_STATE_HEAD IN_STATE_DEFINITION
%x IN_CODE IN_STRING IN_COMMENT IN_MULTILINE_COMMENT

rule_name [A-Z_][A-Z0-9_]*
ws        [ \t\r\v\f]
wsnl      [ \t\r\v\f\n]
value     \"[-a-z]+\"

%option yylineno
%option nodefault
%option noyywrap
%option nounput
%%
    BEGIN IN_DEFINITION_SECTION;

<IN_DEFINITION_SECTION>{
\%\%            {
                    BEGIN IN_RULE_SECTION;
                }
^\%\{           {
                    BEGIN IN_DEFINITION_SECTION_CODE;
                }
\%x             {
                    BEGIN IN_RULE_LIST;
                }
\%option        {
                    BEGIN IN_OPTION_LIST;
                }
\/\*            {
                    definition_section_code_buffer_str += yytext;
                    source_state  =  IN_DEFINITION_SECTION;
                    source_buffer = &definition_section_code_buffer_str;
                    BEGIN IN_MULTILINE_COMMENT;
                }
.               {
                    yyerror("Unknown character encountered inside definition section ('%c') (temp warning)", yytext[0]);
                }
\n              { ; }
}

<IN_RULE_LIST>{
{rule_name}     {
                    rules_map[yytext] = {};
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
\<                  {
                        BEGIN IN_STATE_HEAD;
                    }
.                   {
                        yyerror("Rule section giberish (temp warning) ('%s').", yytext); // XXX
                    }
\n                  { ; }
}

<IN_STATE_HEAD>{
{rule_name} {
                string state_name(yytext);
                map<string, vector<rule_t>>::iterator current_state = rules_map.find(state_name);
                if (current_state == rules_map.end()) {
                    yyerror("State '%s' was never declared.", state_name.c_str());
                } else {
                    current_states.push_back(current_state);
                }
            }
\>\{        {
                patter_buffer = "";
                code_buffer = "";

                BEGIN IN_STATE_DEFINITION;
            }
,           |
{wsnl}      { ; }
.           { 
                yyerror("Unknown character inside state head (%c).", yytext[0]);
            }
}

<IN_STATE_DEFINITION>{
\}          {
                current_states.clear();
                BEGIN IN_RULE_SECTION;
            }
.           {
                patter_buffer += yytext;
            }
\\.         {
                patter_buffer += yytext + 1;
            }
{wsnl}+\{   {
                BEGIN IN_CODE;
                line_buffer = yylineno;
                nest_counter = 0;
            }
\n          { ; }
}

<IN_CODE>{
\{      {
            code_buffer += yytext;
            ++nest_counter;
        }
\}      {
            --nest_counter;
            if (nest_counter == -1) {
                for (const auto &current_state : current_states) {
                    current_state->second.push_back((rule_t) {
                        .state   = -1, // NOTE: initialized elsewhere
                        .pattern = strdup(patter_buffer.c_str()),
                        .code    = strdup(code_buffer.c_str()),
                        .line    = line_buffer,
                    });
                }

                patter_buffer = "";
                code_buffer   = "";

                BEGIN IN_STATE_DEFINITION;
            } else {
                code_buffer += yytext;
            }
        }
\"      {
            code_buffer += yytext;
            BEGIN IN_STRING;
        }
\/\/    {
            code_buffer += yytext;
            BEGIN IN_COMMENT;
        }
\/\*    {
            code_buffer += yytext;
            source_state = IN_CODE;
            source_buffer = &code_buffer;
            BEGIN IN_MULTILINE_COMMENT;
        }
.|\n    {
            code_buffer += yytext;
        }
}

<IN_STRING>{
\"      {
            code_buffer += yytext;
            BEGIN IN_CODE;
        }
\\\\    |
\\\"    |
.|\n    {
            code_buffer += yytext;
        }
}

<IN_COMMENT>{
.       {
            code_buffer += yytext;
        }
\n      {
            code_buffer += yytext;
            BEGIN IN_CODE;
        }
}

<IN_MULTILINE_COMMENT>{
.|\n    {
            *source_buffer += yytext;
        }
\*\/    {
            *source_buffer += yytext;
            BEGIN source_state;
        }
}

<IN_CODE_SECTION>{
(.|\n)* {
            code_section_code_buffer_str += yytext;
        }
}

%%

static
void dump_parse_results(void) {
    fmdictate(stderr, '#', "--- Definition section code buffer ---");
    fmdictate(stderr, '#', definition_section_code_buffer_str.c_str());

    fmdictate(stderr, '&', "--- Code section code buffer ---");
    fmdictate(stderr, '&', code_section_code_buffer_str.c_str());

    fmdictate(stderr, '>', "--- Patterns ---");
    for (const auto &i : rules_map) {
        fmdictatef(stderr, '>', "%s:\n", i.first.c_str());
        for (const auto &h : i.second) {
            fmdictatef(stderr, '>', "\tpattern:\n%s\n" "\tcode:\n%s\n", h.pattern, h.code);
        }
        fmdictate(stderr, '>', "--");
    }
}

static
void dump_rules(void) {
    fdictate(stderr, "--- Rules ---");
    for (rule_t * rule = rules; rule->pattern != NULL; rule++) {
        fdictatef(
            stderr,
            "{ .state = %d, .pattern = `%s` }\n",
            rule->state,
            rule->pattern
        );
    }
    fdictate(stderr, "{ .state = 0, .pattern = NULL }");
}

extern "C"
int parse(const char * filename) {
    // Init
    int r = 0;
    FILE * f = fopen(filename, "r");
    if (!f) { return 2; }

    yyin = f;

    // Parse
    r = yylex();
    if (r) { return r; }

    // Set up globals
    n_rules = 0;
    for (const auto &rule_it : rules_map) {
        n_rules += rule_it.second.size();
    }

    rules = (rule_t*)malloc(sizeof(rule_t)*(n_rules+1));
    rules[n_rules] = (rule_t) { 0, NULL, NULL };

    int index = 0;
    int state = 0;
    for (const auto &rule_it : rules_map) {
        for (const auto &rule : rule_it.second) {
            rules[index++] = (rule_t) {
                .state   = state,
                .pattern = rule.pattern,
                .code    = rule.code,
                .line    = rule.line,
            };
        }
        ++state;
    }

    n_states = rules_map.size();
    state_names = (char**)malloc(sizeof(char*) * n_states);
    int i = 0;
    for (const auto &r : rules_map) {
        state_names[i++] = strdup(r.first.c_str());
    }

    definition_section_code_buffer = strdup(definition_section_code_buffer_str.c_str());
    code_section_code_buffer       = strdup(code_section_code_buffer_str.c_str());

    // Debug
    if (do_debug) {
        dump_parse_results();
        dump_rules();
    }

    return r;
}

extern "C"
int deinit_parser(void) {
    yylex_destroy();
    return 0;
}
