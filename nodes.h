#ifndef PARAMLITE_NODES_H
#define PARAMLITE_NODES_H
typedef enum {
	node_null,
	node_int,
	node_double,
	node_text
} NodeKind;

struct Node {
	NodeKind kind;
	struct Node *next;
	union {
		long long num_int;
		double num_double;
		char *text;
	} data;
};

/*
  Create different kinds of nodes. The next field is initialized as NULL.
  If somehow there's not enough memory to allocate these nodes,
  the program exits. Very unlikely, since it's bound to argv.
 */
struct Node *create_node_int(long long num);
struct Node *create_node_double(double num);
struct Node *create_node_text(char *text);
struct Node *create_node_null(void);

/* Recursively frees all nodes in a linked list */
void destroy_linked_list(struct Node *node);
#endif
