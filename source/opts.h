#ifndef OPS_H
#define OPS_H

#include <stdbool.h>

extern bool do_trace;
extern int do_debug; // NOTE: has multiple levels
extern char * output_filename;
extern char * input_filename;

extern void usage(void);
extern int parse_arguments(const int argc, const char * * argv);
extern void deinit_opts(void);

#endif
