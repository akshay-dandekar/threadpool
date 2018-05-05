#ifndef _LINKED_LIST_S_H_
#define _LINKED_LIST_S_H_

struct linked_list_s_node {
	int size;
	void *val;
	struct linked_list_s_node *next;
};

struct linked_list_s {
	int num_elements;
	struct linked_list_s_node *head;
};

int linked_list_s_init(struct linked_list_s *list);

int linked_list_s_add(struct linked_list_s *list, int pos
		, void *data, int size);

int linked_list_s_del(struct linked_list_s *list, int pos);

int linked_list_s_find_node(struct linked_list_s *list, void *data, int size);

int linked_list_s_get_data(struct linked_list_s *list, int pos, void **data
		, int *size);

int linked_list_s_is_empty(struct linked_list_s *list);

int linked_list_s_size(struct linked_list_s *list);

#endif /* _LINKED_LIST_S_H_ */
