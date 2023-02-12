#include "tests/datastructure.h"
#include "datastructure/linkedlist.h"

struct page_test {
        int data;
        struct list_head head;
};


void test_initialise_linkedlist(void) {
	/* test dynamic init */
	struct page_test *d;
	bool same;

	d = malloc(sizeof(*d));
	list_head_init(&d->head);
	same = d->head.next == &d->head && d->head.prev == &d->head;

	free(d);
	CU_ASSERT(same);

	/* test stack init */
	struct page_test s = {
		.data = 10,
		.head = LIST_HEAD_INIT(s.head)
	};
	CU_ASSERT(&s.head == s.head.next);
	CU_ASSERT(&s.head == s.head.prev);
}

void test_linkedlist_add_tail(void) {
	struct page_test x = {
		.data = 10,
		.head = LIST_HEAD_INIT(x.head)
	};
	
	struct page_test y = {
		.data = 20,
		.head = LIST_HEAD_INIT(y.head)
	};

	list_add_tail(&y.head, &x.head);
	CU_ASSERT(x.head.next == &y.head);
	CU_ASSERT(x.head.prev == &y.head);
	CU_ASSERT(y.head.prev == &x.head);
	CU_ASSERT(y.head.next == &x.head);

	struct page_test z = {
		.data = 30,
		.head = LIST_HEAD_INIT(z.head)
	};
	list_add_tail(&z.head, &y.head);
	CU_ASSERT(x.head.next == &y.head);
	CU_ASSERT(x.head.prev == &z.head);
	CU_ASSERT(y.head.prev == &x.head);
	CU_ASSERT(y.head.next == &z.head);
	CU_ASSERT(z.head.prev == &y.head);
	CU_ASSERT(z.head.next == &x.head);
}

