#ifndef UTIL_H
#define UTIL_H

#define INITIALIZE_ARRAY(a, n, x) do {\
        for (int i = 0; i < n; i++) { \
            a[i] = x;                 \
        }                             \
    } while (0)

#define INITIALIZE_MATRIX(a, n, m, x) do {\
        for (int i = 0; i < n; i++) {     \
            for (int h = 0; h < m; h++) { \
                a[i][h] = x;              \
            }                             \
        }                                 \
    } while (0)

#endif
