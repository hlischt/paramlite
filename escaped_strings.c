#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static int escape_character(int ch) {
	switch (ch) {
	case '0':  return '\0';
	case 'a':  return '\a';
	case 'b':  return '\b';
	// Raw escape, since '\e' is not standard,
	// but we don't care about EBCDIC
	case 'e':  return 0x1B;
	case 'f':  return '\f';
	case 'n':  return '\n';
	case 'r':  return '\r';
	case 't':  return '\t';
	case 'v':  return '\v';
	}
	return ch;
}

size_t convert_escape_sequences(char *dst, char *src, size_t len) {
	size_t dst_i = 0;
	for (size_t i = 0; i < len; i++) {
		if (src[i] == '\\' && i+1 < len) {
			int ch = escape_character(src[i+1]);
			dst[dst_i] = ch;
			i++;
		} else {
			dst[dst_i] = src[i];
		}
		dst_i++;
	}
	dst[dst_i] = '\0';
	return dst_i-1;
}
