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

void test_linkedlist_add(void) {
	struct page_test x = {
		.data = 10,
		.head = LIST_HEAD_INIT(x.head)
	};
	
	struct page_test y = {
		.data = 20,
		.head = LIST_HEAD_INIT(y.head)
	};

	list_add(&y.head, &x.head);
	CU_ASSERT(x.head.next == &y.head);
	CU_ASSERT(x.head.prev == &y.head);
	CU_ASSERT(y.head.prev == &x.head);
	CU_ASSERT(y.head.next == &x.head);

	struct page_test z = {
		.data = 30,
		.head = LIST_HEAD_INIT(z.head)
	};
	list_add(&z.head, &y.head);
	CU_ASSERT(x.head.next == &y.head);
	CU_ASSERT(x.head.prev == &z.head);
	CU_ASSERT(y.head.prev == &x.head);
	CU_ASSERT(y.head.next == &z.head);
	CU_ASSERT(z.head.prev == &y.head);
	CU_ASSERT(z.head.next == &x.head);
}

void test_linkedlist_del(void) {
	struct page_test x = {
		.data = 10,
		.head = LIST_HEAD_INIT(x.head)
	};
	
	struct page_test y = {
		.data = 20,
		.head = LIST_HEAD_INIT(y.head)
	};

	struct page_test z = {
		.data = 30,
		.head = LIST_HEAD_INIT(z.head)
	};
	list_add(&y.head, &x.head);
	list_add(&z.head, &y.head);

	/* delete first */
	list_del(&x.head);
	CU_ASSERT(x.head.next == NULL);
	CU_ASSERT(x.head.prev == NULL);
	CU_ASSERT(y.head.prev == &z.head);
	CU_ASSERT(y.head.next == &z.head);
	CU_ASSERT(z.head.prev == &y.head);
	CU_ASSERT(z.head.next == &y.head);
	list_add(&x.head, &z.head);

	/* delete middle */
	list_del(&y.head);
	CU_ASSERT(y.head.next == NULL);
	CU_ASSERT(y.head.prev == NULL);
	CU_ASSERT(x.head.prev == &z.head);
	CU_ASSERT(x.head.next == &z.head);
	CU_ASSERT(z.head.prev == &x.head);
	CU_ASSERT(z.head.next == &x.head);
	list_add(&y.head, &x.head);

	/* delete end */
	list_del(&z.head);
	CU_ASSERT(z.head.next == NULL);
	CU_ASSERT(z.head.prev == NULL);
	CU_ASSERT(x.head.prev == &y.head);
	CU_ASSERT(x.head.next == &y.head);
	CU_ASSERT(y.head.prev == &x.head);
	CU_ASSERT(y.head.next == &x.head);
}

void test_linkedlist_iterate(void) {
	int total = 0;
	struct list_head *pos;
	struct page_test *entry = NULL;
	LIST_HEAD(head);

	struct page_test x = {
		.data = 10,
		.head = LIST_HEAD_INIT(x.head)
	};

	struct page_test y = {
		.data = 20,
		.head = LIST_HEAD_INIT(y.head)
	};

	struct page_test z = {
		.data = 30,
		.head = LIST_HEAD_INIT(z.head)
	};

	list_add(&x.head, &head);
	list_add(&y.head, &head);
	list_add(&z.head, &head);


	list_for_each(pos, &head) {
		entry = list_entry(pos, typeof(*entry), head);
		total += entry->data;
	}

	CU_ASSERT_EQUAL(total, 60);
}
