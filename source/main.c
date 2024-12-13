#include <stdio.h>
#include <string.h>

#include "opts.h"
#include "parse.h"
#include "jeger.h"

static
char * to_output_name(const char * filename) {
    return strdup("jeger.yy.c"); // XXX temp
}

signed main(const int argc, const char * argv[]) {
    parse_arguments(argc, argv);

    if (!input_filename) {
        puts("No input file specified.");
        usage();
        return 1;
    }

    if (!output_filename) {
        output_filename = to_output_name(input_filename);
    }

    parse(input_filename);
    generate(output_filename);

    deinit_parser();
    deinit_jeger();
    deinit_opts();

    return 0;
}
