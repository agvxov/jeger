#include <stdio.h>

#include "parse.h"
#include "jeger.h"

void usage(void) {
    puts("jeger <file>");
}

signed main(const int argc, char * argv[]) {
    if (argc != 2) {
        usage();
        return 1;
    }

    parse(argv[1]);
    generate("jeger.yy.c");

    deinit_parser();
    deinit_jeger();

    return 0;
}
