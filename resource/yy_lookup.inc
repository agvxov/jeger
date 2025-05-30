static inline
int mlookup(const char * s, int state) {
    for (int i = 0; s[i] != '\0'; i++) {
        state = table[state][AS_SYMBOL(s[i])];
        if (state == NO_MATCH) {
            break;
        } else
        if (state > NO_MATCH) {
            mlen = i+1;
            return state;
        }
    }

    mlen = 0;
    return NO_MATCH;
}
