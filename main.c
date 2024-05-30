#define _POSIX_C_SOURCE 200809L
#define PARAMLITE_VERSION "0.1"
#define EXIT_CLI_ERROR 2
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>

#include "nodes.h"
#include "escaped_strings.h"

const char * const usage_str =
"Usage: paramlite [OPTIONS...] [ORDERED PARAMETERS...] DATABASE... < QUERY\n"
"Database options:\n"
"	-r        Open database for read-only access (default)\n"
"	-w        Open database for writing\n"
"	-c        Create database if it doesn't exist; implies -w\n"
"Output options:\n"
"	-F <sep>  Output field separator (default \"\\t\")\n"
"	-R <sep>  Output record separator (default \"\\n\")\n"
"Other options:\n"
"	-v        Show version information\n"
"	-h        Show this help\n"
"Parameters:\n"
"	-n        NULL\n"
"	-d <num>  Integer (64-bit)\n"
"	-f <num>  Double-precision floating point number\n"
"	-t <str>  Text\n";

/*
  Create a new string based on src with escape sequences evaluated,
  and make sep_str point to it. Don't forget to free(*sep_str)
 */
void change_sep(char **sep_str, char *src) {
	size_t len = strlen(src);
	char *dst = malloc(len);
	convert_escape_sequences(dst, src, len);
	*sep_str = dst;
}

/*
  Replace curr with the new node, updating the next field if it's NULL.
  Additionally, change start only if it's a null pointer.
*/
void cycle_curr_node(struct Node *node,
		     struct Node **curr,
		     struct Node **start) {
	if (*curr != NULL) {
		(*curr)->next = node;
	}
	*curr = node;
	if (*start == NULL) {
		*start = node;
	}
}

void parse_int(char *optstr, struct Node **curr, struct Node **start) {
	char *last = NULL;
	long long d = strtoll(optstr, &last, 0);
	// Whole parameter should be a number,
	// lest it could lead to weird behavior
	if (*last != '\0') {
		fprintf(stderr, "-d: \"%s\" is not "
			"a valid integer\n", optstr);
		exit(EXIT_CLI_ERROR);
	}
	if (errno != 0) {
		fprintf(stderr, "-d %s: Number doesn't fit "
			"in a 64-bit integer\n", optstr);
		exit(EXIT_CLI_ERROR);
	}
	struct Node *node = create_node_int(d);
	cycle_curr_node(node, curr, start);
}

void parse_double(char *optstr, struct Node **curr, struct Node **start) {
	char *last = NULL;
	double f = strtod(optstr, &last);
	// Whole parameter should be a number,
	// lest it could lead to weird behavior
	if (*last != '\0') {
		fprintf(stderr, "-f: \"%s\" is not "
			"a valid floating point number\n", optstr);
		exit(EXIT_CLI_ERROR);
	}
	if (errno != 0) {
		perror("-f");
		exit(EXIT_CLI_ERROR);
	}
	struct Node *node = create_node_double(f);
	cycle_curr_node(node, curr, start);
}

void parse_text(char *optstr, struct Node **curr, struct Node **start) {
	struct Node *node = create_node_text(optstr);
	cycle_curr_node(node, curr, start);
}

void parse_null(struct Node **curr, struct Node **start) {
	struct Node *node = create_node_null();
	cycle_curr_node(node, curr, start);
}

void walk_list(struct Node *node) {
	int c = 1;
	while (node != NULL) {
		switch (node->kind) {
		case node_null:
			printf("Param %d: NULL\n", c++);
			break;
		case node_int:
			printf("Param %d: %lld\n", c++, node->data.num_int);
			break;
		case node_double:
			printf("Param %d: %f\n", c++, node->data.num_double);
			break;
		case node_text:
			printf("Param %d: %s\n", c++, node->data.text);
			break;
		}
		node = node->next;
	}
}

int main(int argc, char **argv) {
	int open_mode = SQLITE_OPEN_READONLY;
	char *field_sep = "\t";
	char *record_sep = "\n";
	bool fs_malloc = false;
	bool rs_malloc = false;
	struct Node *start = NULL;
	struct Node *curr  = start;
	int c;
	while ((c = getopt(argc, argv, ":rwcvhF:R:nd:f:t:")) != -1) {
		switch (c) {
		case 'r': open_mode = SQLITE_OPEN_READONLY;
			break;
		case 'w': open_mode &= ~SQLITE_OPEN_READONLY;
			open_mode |= SQLITE_OPEN_READWRITE;
			break;
		case 'c': open_mode = SQLITE_OPEN_READWRITE |
				SQLITE_OPEN_CREATE;
			break;
		case 'v': printf("paramlite %s\n", PARAMLITE_VERSION);
			return EXIT_SUCCESS;
		case 'h': fputs(usage_str, stderr);
			return EXIT_SUCCESS;
		case 'F': change_sep(&field_sep, optarg);
			fs_malloc = true;
			break;
		case 'R': change_sep(&record_sep, optarg);
			rs_malloc = true;
			break;
		case 'n':
			parse_null(&curr, &start);
			break;
		case 'd':
			parse_int(optarg, &curr, &start);
			break;
		case 'f':
			parse_double(optarg, &curr, &start);
			break;
		case 't':
			parse_text(optarg, &curr, &start);
			break;
		case ':': printf("Error: option -%c requires an operand\n",
				 optopt);
			return EXIT_CLI_ERROR;
		default:
			printf("Bad argument: -%c\n", c);
			fputs(usage_str, stderr);
			return EXIT_CLI_ERROR;
			break;
		}
	}
	printf("Open mode: %d\n", open_mode);
	walk_list(start);
	if (rs_malloc) free(record_sep);
	if (fs_malloc) free(field_sep);
	if (start != NULL) destroy_linked_list(start);
	return EXIT_SUCCESS;
}
