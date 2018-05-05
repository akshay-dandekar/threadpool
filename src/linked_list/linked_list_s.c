#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "linked_list_s.h"

int linked_list_s_init(struct linked_list_s *list) {
	if(list == NULL) {
		fprintf(stderr, "[%s,ERROR] Invalid list pointer\r\n"
				, __FUNCTION__);
		return -1;
	}

	list->num_elements = 0;
	list->head = NULL;

	return 0;
}

int linked_list_s_add(struct linked_list_s *list, int pos
		, void *data, int size) {
	struct linked_list_s_node *node, *ptr_cur;

	if(list == NULL) {
		fprintf(stderr, "[%s,ERROR] Invalid list pointer\r\n"
				, __FUNCTION__);
		return -1;
	}

	if(size <= 0) {
		fprintf(stderr, "[%s, ERROR] Invalid size of data\r\n", __FUNCTION__);
		return -1;
	}

	if(data == NULL) {
		fprintf(stderr, "[%s, ERROR] Invalid data pointer\r\n", __FUNCTION__);
		return -1;
	}
	
	if(pos > list->num_elements) {
		fprintf(stderr, "[%s, ERROR] Invalid position\r\n", __FUNCTION__);
		return -1;
	}

	/* Create new node */
	node = (struct linked_list_s_node *) 
		malloc(sizeof(struct linked_list_s_node));
	if(node == NULL) {
		fprintf(stderr, "[%s, ERROR] No memory for new node\r\n", __FUNCTION__);
		return -1;
	}

	/* Set node data */
	node->size = size;
	node->next = NULL;
	node->val = data;
//		malloc(size);
//	if(node->val == NULL) {
//		fprintf(stderr, "[%s, ERROR] No memory for node data\r\n"
//				, __FUNCTION__);
//		free(node);
//		return -1;
//	}
//	memcpy(node->val, data, size);

	/* Set the position variable to end of list if pos < 0 */
	if(pos < 0) {
		pos = list->num_elements;
	}


	/* If list is empty */
	if(list->num_elements == 0) {
		list->head = node;
		list->num_elements++;

		return 0;
	}

	/* Goto the given position */
	ptr_cur = list->head;
	while(pos > 1) {
		ptr_cur = ptr_cur->next;
		pos--;
	}

	node->next = ptr_cur->next;
	ptr_cur->next = node;

	list->num_elements++;
	
	return 0;
}

int linked_list_s_del(struct linked_list_s *list, int pos) {
	struct linked_list_s_node *ptr_cur, *ptr_del;
	int count;

	if(list == NULL) {
		fprintf(stderr, "[%s,ERROR] Invalid list pointer\r\n"
				, __FUNCTION__);
		return -1;
	}

	if(list->num_elements == 0) {
		fprintf(stderr, "[%s, ERROR] List is empty\n\r", __FUNCTION__);
		return -1;
	}

	if((pos >= list->num_elements) || (pos < 0)) {
		fprintf(stderr, "[%s, ERROR] Invalid position\r\n", __FUNCTION__);
		return -1;
	}

	/* If first element to delete */
	ptr_cur = list->head;

	if(list->num_elements == 1) {
		list->head = NULL;

		/* Free node memory and delete the node */
		free(ptr_cur);

		list->num_elements--;
		return 0;
	} else if(list->num_elements == 2) {
		if(pos == 0) {
			list->head = ptr_cur->next;
		} else {
			ptr_cur = list->head->next;
			list->head->next = NULL;
		}

		free(ptr_cur);
		list->num_elements--;
		return 0;
	} else if(pos == 0) {
		list->head = ptr_cur->next;

//		free(ptr_cur->val);
		free(ptr_cur);

		list->num_elements--;
		return 0;
	}

	count = pos;
	while(count > 1) {
		ptr_cur = ptr_cur->next;
		count--;
	}

	ptr_del = ptr_cur->next;

	if(pos == (list->num_elements - 1)) {
		ptr_cur->next = NULL;
	} else {
		ptr_cur->next = ptr_cur->next->next;
	}

	/* Free node memory and delete the node */
//	free(ptr_del->val);
	free(ptr_del);

	list->num_elements--;
	return 0;
}

int linked_list_s_find_node(struct linked_list_s *list, void *data, int size) {
	struct linked_list_s_node *ptr_cur;
	int ret, pos;

	if(list == NULL) {
		fprintf(stderr, "[%s,ERROR] Invalid list pointer\r\n"
				, __FUNCTION__);
		return -1;
	}

	if(size <= 0) {
		fprintf(stderr, "[%s, ERROR] Invalid size of data\r\n", __FUNCTION__);
		return -1;
	}

	if(data == NULL) {
		fprintf(stderr, "[%s, ERROR] Invalid data pointer\r\n", __FUNCTION__);
		return -1;
	}

	if(list->num_elements == 0) {
		fprintf(stderr, "[%s, ERROR] List is empty\n\r", __FUNCTION__);
		return -1;
	}

	ret = -1;
	pos = 0;
	ptr_cur = list->head;
	while(pos < list->num_elements) {
		if(ptr_cur->size == size) {
			if(ptr_cur->val == data) {
//			if(memcmp(ptr_cur->val, data, ptr_cur->size) == 0) {
				ret = pos;
				break;
			}
		}
		pos++;
		ptr_cur = ptr_cur->next;
	}

	return ret;
}

int linked_list_s_get_data(struct linked_list_s *list, int pos, void **data
		, int *size) {
	struct linked_list_s_node *ptr_cur;

	if(list == NULL) {
		fprintf(stderr, "[%s,ERROR] Invalid list pointer\r\n"
				, __FUNCTION__);
		return -1;
	}

	if(size == NULL) {
		fprintf(stderr, "[%s, ERROR] Invalid size ptr\r\n", __FUNCTION__);
		return -1;
	}

	if(data == NULL) {
		fprintf(stderr, "[%s, ERROR] Invalid data pointer\r\n", __FUNCTION__);
		return -1;
	}

	if(list->num_elements == 0) {
		fprintf(stderr, "[%s, ERROR] List is empty\n\r", __FUNCTION__);
		return -1;
	}

	if((pos >= list->num_elements) || (pos < 0)) {
		fprintf(stderr, "[%s, ERROR] Invalid position\r\n", __FUNCTION__);
		return -1;
	}

	ptr_cur = list->head;
	while(pos > 0) {
		ptr_cur = ptr_cur->next;
		pos--;
	}

//	memcpy(data, ptr_cur->val, ptr_cur->size);
	*data = ptr_cur->val;
	*size = ptr_cur->size;

	return 0;
}

int linked_list_s_is_empty(struct linked_list_s *list) {
	return (list->num_elements == 0);
}

int linked_list_s_size(struct linked_list_s *list) {
	return list->num_elements;
}
