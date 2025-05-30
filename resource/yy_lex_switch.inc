    // XXX
    int yylex(const char * s) {
        int state = 0;
        for (
          const char * ss = s;
          *ss != '\00';
          ss += ((mlen ? mlen : 1) * direction)
        ) {
            int match;
            switch (state) {
                {{ code }}
        }
    }
    return 0;
}
