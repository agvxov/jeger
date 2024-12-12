#ifndef OPS_H
#define OPS_H

#include <stdbool.h>

extern bool do_trace;
extern bool do_debug;
extern char * output_filename;
extern char * input_filename;

extern void usage(void);
extern int parse_arguments(const int argc, const char * * argv);

#endif
