#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char *data;     // Pointer to buffer data
    size_t size;    // Current buffer size
} buffer;

// Create and return a new empty buffer
buffer buffer_init(void);

// Free buffer memory
void buffer_destroy(buffer *buffer);

// Append data to buffer
void buffer_add(buffer *buffer, const char *data, size_t data_size);

// Check if buffer is empty
int buffer_is_empty(buffer *buffer);

// Find exact data in buffer; return position or -1 if not found
int buffer_find(buffer *buffer, const char *data, size_t data_size);

// Find data (case-insensitive); return position or -1 if not found
int buffer_find_insensitive(buffer *buffer, const char *data, size_t data_size);

#endif
