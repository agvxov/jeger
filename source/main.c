#include "parse.h"

void usage(void) {
    puts("jeger <file>");
}

signed main(const int argc, char * argv[]) {
    if (argc != 2) {
        usage();
        return 1;
    }

    parse(argv[1]);

    return 0;
}
