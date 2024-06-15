#ifndef PARAMLITE_ESCAPED_STRINGS_H
#define PARAMLITE_ESCAPED_STRINGS_H
#include <stddef.h>
size_t convert_escape_sequences(char *dst, char *src, size_t len);
size_t escape_string(unsigned char **dst, const unsigned char *src, size_t len);
#endif
