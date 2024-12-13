#include "opts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "jeger.h"

bool do_trace = false;
int do_debug = 0;

char * output_filename = NULL;
char * input_filename  = NULL;

void usage(void) {
    puts(
        "jeger [options] <file>  : the jeger scanner generator\n"
        "\t-h        : print help and exit\n"
        "\t-t        : enable trace mode in the generated scanner\n"
        "\t-d        : enable debug mode during generating\n"
        "\t-o <file> : specify output file name"
    );
}

int parse_arguments(const int argc, const char * * argv) {
    int opt;
    const char * output_file = NULL;

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"help",   no_argument,       0, 'h'},
        {"trace",  no_argument,       0, 't'},
        {"debug",  no_argument,       0, 'd'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, (char * const *)argv, "o:htd", long_options, NULL)) != -1) {
        switch (opt) {
            case '?': exit(1);
            case 'h': {
                usage();
            } exit(0);
            case 'o': {
                output_filename = strdup(optarg);
            } break;
            case 't': {
                do_trace = true;
            } break;
            case 'd': {
                ++do_debug;
            } break;
            default: {
                usage();
            } exit(1);
        }
    }

    if (optind < argc) { // XXX
        input_filename = strdup(argv[optind]);
    }

    return 0;
}

void deinit_opts(void) {
    free(output_filename);
    free(input_filename);
}
