#include <tests/datastructure.h>
#include <datastructure/stack.h>
#include <datastructure/queue.h>

void test_queue_init(void)
{
	struct queue ct = {0};
	CU_ASSERT(queue_init(&ct));
	CU_ASSERT_PTR_NOT_NULL(ct.arr);
	queue_free(&ct);
}

void test_queue_offer(void)
{
	struct queue ct = {0};

	char *str = "test123456789012345678901234567890";

	CU_ASSERT(queue_init(&ct));

	for (int i = 0; i < 50; i++) {
		CU_ASSERT(queue_offer(&ct, str, strlen(str) + 1));
		CU_ASSERT_EQUAL(ct.arr->len, (i + 1) * sizeof(uintptr_t));
	}

	queue_free(&ct);
}

void test_queue_poll(void)
{
	struct queue ct = {0};

	char *str = "test123456789012345678901234567890";

	CU_ASSERT(queue_init(&ct));

	for (int i = 0; i < 50; i++) {
		CU_ASSERT(queue_offer(&ct, str, strlen(str) + 1));
		CU_ASSERT_EQUAL(ct.arr->len, (i + 1) * sizeof(uintptr_t));
	}

	for (int i = 49; i >= 0; i--) {
		char *pop_ptr = (char*)queue_poll(&ct);
		CU_ASSERT_STRING_EQUAL(pop_ptr, str);
		CU_ASSERT_EQUAL(ct.arr->len, i * sizeof(uintptr_t));
		/* once content is popped from queue, it's the caller's
		 * responsibility to free it */
		free(pop_ptr);
	}

	CU_ASSERT_EQUAL(ct.arr->len, 0);
	CU_ASSERT_PTR_NOT_NULL(ct.arr);

	queue_free(&ct);
}

void test_queue_free(void)
{
	struct queue ct = {0};
	CU_ASSERT(queue_init(&ct));
	queue_free(&ct);
	CU_ASSERT_PTR_NULL(ct.arr);
}

void test_queue_peek(void)
{
	struct queue ct = {0};

	CU_ASSERT(queue_init(&ct));

	for (int i = 0; i < 3; i++) {
		CU_ASSERT(queue_offer(&ct, &i, sizeof(int)));
		CU_ASSERT_EQUAL(ct.arr->len, (i + 1) * sizeof(uintptr_t));
	}

	CU_ASSERT_EQUAL(*(int* )queue_peek(&ct), 0);
	CU_ASSERT_EQUAL(ct.arr->len, 3 * sizeof(uintptr_t));

	queue_free(&ct);
}

void test_queue_peek_pos(void)
{
	struct queue ct = {0};

	CU_ASSERT(queue_init(&ct));

	for (int i = 0; i < 3; i++) {
		CU_ASSERT(queue_offer(&ct, &i, sizeof(int)));
		CU_ASSERT_EQUAL(ct.arr->len, (i + 1) * sizeof(uintptr_t));
	}

	for (int i = 0; i < 3; i++) {
		CU_ASSERT_EQUAL(*(int* )queue_peek_pos(&ct, i), i);
	}
	CU_ASSERT_EQUAL(ct.arr->len, 3 * sizeof(uintptr_t));

	queue_free(&ct);
}
