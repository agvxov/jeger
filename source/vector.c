#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void vector_init(vector_t *        vector,
                 size_t      element_size,
                 size_t     element_count) {
	assert(element_size);

	vector->data          = NULL;
	vector->element_size  = element_size;
	vector->element_count = element_count;

	vector->data = (char *)calloc(vector->element_count, vector->element_size);

	assert(vector->data);
}

void vector_push(vector_t * vector,
                 void     *   data) {
	assert(vector);

	vector->element_count += 1;

	vector->data = (char *)realloc(vector->data,
	                               vector->element_size * vector->element_count);

	assert(vector->data);

	memcpy(&vector->data[(vector->element_count - 1) * vector->element_size],
	       data,
	       vector->element_size);
}

void vector_pop(vector_t * vector) {
	assert(vector); // UNUSED
}

void * vector_get(const vector_t * const  vector,
                  const size_t           element) {
	assert(vector);
	assert(element < vector->element_count);

	return &vector->data[vector->element_size * element];
}

void vector_set(vector_t *  vector,
                void     *    data,
                size_t     element) {
	assert(vector);
	assert(element >= vector->element_count);

	memcpy(&vector->data[vector->element_size * element],
	       data,
	       vector->element_size);
}

void vector_free(vector_t * vector) {
	free(vector->data);
}

