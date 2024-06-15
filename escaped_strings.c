#include <ctype.h>
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

static int character_escape(unsigned char ch) {
	switch (ch) {
	case '\0':  return '0';
	case '\a':  return 'a';
	case '\b':  return 'b';
	// Raw escape, since '\e' is not standard,
	// but we don't care about EBCDIC
	case 0x1B:  return 'e';
	case '\f':  return 'f';
	case '\n':  return 'n';
	case '\r':  return 'r';
	case '\t':  return 't';
	case '\v':  return 'v';
	case '\\':  return '\\';
	}
	return -1;
}

/*
  Read a string src[len] and convert escape sequences (\0, \n, \\, etc.)
  to the bytes they represent, writing the result to dst.

  dst must be already allocated, and its length should be the same as src,
  that is, len, in case src has no escaped sequences. sizeof(dst) <= sizeof(src)
 */
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

static unsigned char hex_digit(unsigned char ch) {
	return ch <= 9 ? ch + '0' : 10 <= ch && ch <= 15 ? ch + 'A' : '?';
}

size_t escape_string(unsigned char **dst, const unsigned char *src, size_t len) {
	if (len == 0) {
		*dst = calloc(1, 1);
		if (*dst == NULL) {
			perror(__func__);
			exit(1);
		}
		return 0;
	}
	// An escaped byte uses up to four chars (\xXX), so a buffer full
	// of escaped bytes will be four times as long as src.
	// + 1 to include the null terminator.
	unsigned char *buf = malloc(len * 4 + 1);
	size_t buf_i = 0;
	if (buf == NULL) {
		perror(__func__);
		exit(1);
	}
	for (size_t i = 0; i < len; i++) {
		if (isprint(src[i])) {
			buf[buf_i] = src[i];
			buf_i++;
		} else if (character_escape(src[i]) != -1) {
			buf[buf_i] = '\\';
			buf[buf_i+1] = character_escape(src[i]);
			buf_i += 2;
		} else {
			buf[buf_i] = '\\';
			buf[buf_i+1] = 'x';
			buf[buf_i+2] = hex_digit((src[i] & 0xF0) >> 4);
			buf[buf_i+3] = hex_digit(src[i] & 0x0F);
			buf_i += 4;
		}
	}
	buf[buf_i] = '\0';
	*dst = buf;
	return buf_i;
}
