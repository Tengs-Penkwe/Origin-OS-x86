#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"

#define offset(strut_type,member) (int)(&((strut_type*)0)->member)
#define elem2entry(strut_type, struct_member_name, elem_ptr) \
		(strut_type*)((int)elem_ptr - offset(strut_type, struct_member_name))

struct list_elem {
	struct list_elem* prev;
	struct list_elem* next;
};

struct list {
	struct list_elem head;
	struct list_elem tail;
};

/** Self-defined type function, used as return funtion in list_traversal **/
typedef bool (function)(struct list_elem*, int arg);

void list_init (struct list*);
void list_insert_before(struct list_elem* before, struct list_elem* elem);
void list_push(struct list* plist, struct list_elem* elem);
void list_iterate(struct list* plist);
void list_append(struct list* plist, struct list_elem* elem);
void list_remove(struct list_elem* pelem);
struct list_elem* list_pop(struct list* plist);
bool elem_find(struct list* plist, struct list_elem* obj_elem);
struct list_elem* list_traversal(struct list* plist, function func, int arg);
uint32_t list_len(struct list* plist);
bool list_empty(struct list* plist);

#endif //__LIB_KERNEL_LIST_H
