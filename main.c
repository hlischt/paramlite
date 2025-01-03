#define _POSIX_C_SOURCE 200809L
#define PARAMLITE_VERSION "1.0"
#define EXIT_CLI_ERROR 2
#define CHARBUF_SIZE 4096
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
"	-N <str>  Stand-in for NULL values (default \"[NULL]\")\n"
"	-B <str>  Stand-in for BLOB values\n"
"Other options:\n"
"	-v        Show version information\n"
"	-h        Show this help\n"
"Parameters:\n"
"	-n        NULL\n"
"	-d <num>  Integer (64-bit)\n"
"	-f <num>  Double-precision floating point number\n"
"	-t <str>  Text\n";

struct settings {
	int open_mode;
	char *field_sep;
	char *record_sep;
	char *null_str;
	char *blob_str;
};

/*
  Create a new string based on src with escape sequences evaluated,
  and make sep_str point to it. Don't forget to free(*sep_str)
 */
void change_sep(char **sep_str, char *src) {
	size_t len = strlen(src);
	char *dst = malloc(len + 1);
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

void bind_list(struct Node *node, sqlite3_stmt *stmt) {
	int idx = 1;
	int err = 0;
	while (node != NULL) {
		switch (node->kind) {
		case node_null:
			err = sqlite3_bind_null(stmt, idx);
			if (err != SQLITE_OK) {
				fprintf(stderr, "-n: %s\n",
					sqlite3_errstr(err));
			}
			break;
		case node_int:
			err = sqlite3_bind_int64(stmt, idx,
						 node->data.num_int);
			if (err != SQLITE_OK) {
				fprintf(stderr, "-d %lld: %s\n",
					node->data.num_int,
					sqlite3_errstr(err));
			}
			break;
		case node_double:
			err = sqlite3_bind_double(stmt, idx,
						  node->data.num_double);
			if (err != SQLITE_OK) {
				fprintf(stderr, "-f %f: %s\n",
					node->data.num_double,
					sqlite3_errstr(err));
			}
			break;
		case node_text:
			err = sqlite3_bind_text(stmt, idx,
						node->data.text,
						-1, SQLITE_STATIC);
			if (err != SQLITE_OK) {
				fprintf(stderr, "-t %s: %s\n",
					node->data.text,
					sqlite3_errstr(err));
			}
			break;
		}
		idx++;
		node = node->next;
	}
}

// Read the entire standard input into a buffer and return its size.
// Don't forget to free the buffer.
size_t slurp_stdin(char **dst) {
	size_t n = 0;
	size_t bufsize = 0;
	char *buf = malloc(CHARBUF_SIZE);
	while ((n = fread(buf, 1, CHARBUF_SIZE, stdin)) == CHARBUF_SIZE) {
		bufsize += n;
		buf = realloc(buf, bufsize + CHARBUF_SIZE);
		if (buf == NULL) {
			perror(__func__);
			exit(EXIT_FAILURE);
		}
	}
	bufsize += n;
	*dst = buf;
	return bufsize;
}

#define SQLITE_ERR_HANDLE(after_stmt)					\
	if ((retcode) != SQLITE_OK) {					\
		errmsg = sqlite3_errmsg((db));				\
		fprintf(stderr, "%s: %s\n", db_path, errmsg);		\
		if (after_stmt) sqlite3_finalize(stmt);			\
		sqlite3_close(db);					\
		return retcode;						\
	}

int execute_query(struct Node *list, char *db_path,
		  struct settings *settings,
		  char *query, size_t query_len) {
	int retcode = SQLITE_OK;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *tail = NULL;
	const char *errmsg = NULL;
	retcode = sqlite3_open_v2(db_path, &db, settings->open_mode, NULL);
	SQLITE_ERR_HANDLE(false);
	retcode = sqlite3_prepare_v2(db, query, query_len, &stmt, &tail);
	SQLITE_ERR_HANDLE(true);
	bind_list(list, stmt);
	int cols = sqlite3_column_count(stmt);
	while ((retcode = sqlite3_step(stmt)) == SQLITE_ROW) {
		for (int i = 0; i < cols; i++) {
			switch (sqlite3_column_type(stmt, i)) {
			case SQLITE_NULL:
				printf("%s", settings->null_str);
				break;
			case SQLITE_INTEGER:
				printf("%lld", sqlite3_column_int64(stmt, i));
				break;
			case SQLITE_FLOAT:
				printf("%f", sqlite3_column_double(stmt, i));
				break;
			case SQLITE_TEXT:
				printf("%s", sqlite3_column_text(stmt, i));
				break;
			case SQLITE_BLOB:
				if (settings->blob_str != NULL) {
					printf("%s", settings->blob_str);
				} else {
					unsigned char *escaped = NULL;
					const unsigned char *blob = NULL;
					size_t len, esc_len;
					blob = sqlite3_column_blob(stmt, i);
					len = sqlite3_column_bytes(stmt, i);
					esc_len = escape_string(&escaped,
								blob, len);
					printf("%.*s", (int)esc_len, escaped);
					free(escaped);
					escaped = NULL;
				}
				break;
			}
			printf("%s", i == cols-1
			       ? settings->record_sep
			       : settings->field_sep);
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	if (retcode == SQLITE_MISMATCH) {
		fprintf(stderr, "SQL query error: %s (%s)\n",
			sqlite3_errstr(retcode),
			"check the number of parameters and their types");
		return retcode;
	}
	return SQLITE_OK;
}

int main(int argc, char **argv) {
	struct settings settings = {
		.open_mode = SQLITE_OPEN_READONLY,
		.field_sep = "\t",
		.record_sep = "\n",
		.null_str = "[NULL]",
		.blob_str = NULL,
	};
	bool fs_malloc = false;
	bool rs_malloc = false;
	bool null_malloc = false;
	struct Node *start = NULL;
	struct Node *curr  = start;
	int c;
	while ((c = getopt(argc, argv, ":rwcvhF:R:N:B:nd:f:t:")) != -1) {
		switch (c) {
		case 'r': settings.open_mode = SQLITE_OPEN_READONLY;
			break;
		case 'w': settings.open_mode &= ~SQLITE_OPEN_READONLY;
			settings.open_mode |= SQLITE_OPEN_READWRITE;
			break;
		case 'c': settings.open_mode = SQLITE_OPEN_READWRITE |
				SQLITE_OPEN_CREATE;
			break;
		case 'v': printf("paramlite %s\n", PARAMLITE_VERSION);
			return EXIT_SUCCESS;
		case 'h': fputs(usage_str, stderr);
			return EXIT_SUCCESS;
		case 'F': change_sep(&(settings.field_sep), optarg);
			fs_malloc = true;
			break;
		case 'R': change_sep(&(settings.record_sep), optarg);
			rs_malloc = true;
			break;
		case 'N':
			change_sep(&(settings.null_str), optarg);
			null_malloc = true;
			break;
		case 'B':
			change_sep(&(settings.blob_str), optarg);
			// No blob_malloc variable because the default is NULL,
			// which means no buffer was allocated.
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
	if (optind >= argc) {
		fputs(usage_str, stderr);
		fprintf(stderr, "\nError: No database provided.\n");
		return EXIT_CLI_ERROR;
	}
	char *query = NULL;
	size_t query_size = slurp_stdin(&query);
	int retcode = SQLITE_OK;
	for (int i = optind; i < argc; i++) {
		retcode = execute_query(start, argv[i], &settings,
					query, query_size);
		if (retcode == SQLITE_MISMATCH) {
			// Mismatched parameters,
			// query won't work with any database
			break;
		}
	}
	if (query != NULL) free(query);
	if (rs_malloc) free(settings.record_sep);
	if (fs_malloc) free(settings.field_sep);
	if (null_malloc) free(settings.null_str);
	if (settings.blob_str != NULL) free(settings.blob_str);
	if (start != NULL) destroy_linked_list(start);
	if (retcode == SQLITE_MISMATCH) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
