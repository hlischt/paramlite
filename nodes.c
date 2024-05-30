#include <stdio.h>
#include <stdlib.h>

#include "nodes.h"

struct Node *create_node_int(long long num) {
	struct Node *node = malloc(sizeof(struct Node));
	if (node == NULL) {
		perror("create_node_int");
		exit(1);
	}
	node->kind = node_int;
	node->next = NULL;
	node->data.num_int = num;
	return node;
}

struct Node *create_node_double(double num) {
	struct Node *node = malloc(sizeof(struct Node));
	if (node == NULL) {
		perror("create_node_double");
		exit(1);
	}
	node->kind = node_double;
	node->next = NULL;
	node->data.num_double = num;
	return node;
}

struct Node *create_node_text(char *text) {
	struct Node *node = malloc(sizeof(struct Node));
	if (node == NULL) {
		perror("create_node_text");
		exit(1);
	}
	node->kind = node_text;
	node->next = NULL;
	node->data.text = text;
	return node;
}

struct Node *create_node_null(void) {
	struct Node *node = malloc(sizeof(struct Node));
	if (node == NULL) {
		perror("create_node_null");
		exit(1);
	}
	node->kind = node_null;
	node->next = NULL;
	return node;
}

void destroy_linked_list(struct Node *node) {
	if (node == NULL) return;
	struct Node *current = node;
	struct Node *next = NULL;
	do {
		next = current->next;
		free(current);
		current = next;
	} while (current != NULL);
}
