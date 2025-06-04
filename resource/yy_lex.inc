int yylex(const char * s) {
    int state = 0;
    for (
      const char * ss = s;
      *ss != '\0';
      ss += ((mlen ? mlen : 1) * direction)
    ) {
        int match = mlookup(ss, state);
        switch (match) {
            {{ code }}
        }
    }
    return 0;
}
