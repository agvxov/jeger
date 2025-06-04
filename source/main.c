#include <stdio.h>
#include <string.h>

#include "opts.h"
#include "parse.h"
#include "jeger.h"

static
char * to_output_name(const char * filename) {
    size_t filename_len = strlen(filename);
    size_t buffer_size = filename_len + sizeof(".yy.");
    char buffer[buffer_size];

    const char * last_slash = strrchr(filename, '/');
    const char * basename = (last_slash ? last_slash + 1 : filename);
    const char * dot = strrchr(basename, '.');

    if (dot) {
        // Insert ".yy." before the extension
        size_t prefix_len = dot - filename;
        memcpy(buffer, filename, prefix_len);
        memcpy(buffer + prefix_len, ".yy.", 4);
        strcpy(buffer + prefix_len + 4, dot + 1);
    } else {
        // No extension, append ".yy"
        memcpy(buffer, filename, filename_len);
        memcpy(buffer + filename_len, ".yy", 4);
    }

    return strdup(buffer);
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
