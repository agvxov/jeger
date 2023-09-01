#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

// TODO: Handle error warnings?
// TODO: Implement more useful functions?

typedef struct {
	char   *          data;
	size_t    element_size;
	size_t   element_count;
} vector_t;

extern void vector_init(vector_t *        vector,
                        size_t      element_size,
                        size_t     element_count);

extern void vector_push(vector_t * vector,
                        void     *   data);

extern void vector_pop(vector_t * vector);

extern void * vector_get(const vector_t * const   vector,
                         const size_t            element);

extern void vector_set(vector_t *  vector,
                       void     *    data,
                       size_t     element);

extern void vector_free(vector_t * vector);

#endif
