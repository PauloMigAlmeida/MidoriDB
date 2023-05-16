#include <tests/datastructure.h>
#include <datastructure/stack.h>

void test_stack_init(void)
{
	struct stack st = {0};
	CU_ASSERT(stack_init(&st));
	CU_ASSERT_EQUAL(st.idx, -1);
	CU_ASSERT_PTR_NOT_NULL(st.arr);
	stack_free(&st);
}

void test_stack_push(void)
{
	struct stack st = {0};

	char *str = "test123456789012345678901234567890";

	CU_ASSERT(stack_init(&st));

	for (int i = 0; i < 50; i++) {
		CU_ASSERT(stack_push(&st, str, strlen(str) + 1));
		CU_ASSERT_EQUAL(st.idx, i);
	}

	stack_free(&st);
}

void test_stack_pop(void)
{
	struct stack st = {0};

	char *str = "test123456789012345678901234567890";

	CU_ASSERT(stack_init(&st));

	for (int i = 0; i < 50; i++) {
		CU_ASSERT(stack_push(&st, str, strlen(str) + 1));
		CU_ASSERT_EQUAL(st.idx, i);
	}

	for (int i = 49; i >= 0; i--) {
		char *pop_ptr = (char*)stack_pop(&st);
		CU_ASSERT_STRING_EQUAL(pop_ptr, str);
		CU_ASSERT_EQUAL(st.idx, i - 1);
		/* once content is popped form stack, it's the caller's
		 * responsibility to free it */
		free(pop_ptr);
	}

	CU_ASSERT_EQUAL(st.idx, -1);
	CU_ASSERT_PTR_NOT_NULL(st.arr);

	stack_free(&st);
}

void test_stack_free(void)
{
	struct stack st = {0};
	CU_ASSERT(stack_init(&st));
	stack_free(&st);
	CU_ASSERT_EQUAL(st.idx, -1);
}

void test_stack_peek(void)
{
	struct stack st = {0};
	
	CU_ASSERT(stack_init(&st));
	
	for (int i = 0; i < 3; i++) {
		CU_ASSERT(stack_push(&st, &i, sizeof(int)));
		CU_ASSERT_EQUAL(st.idx, i);
	}

	CU_ASSERT_EQUAL(*(int*)stack_peek(&st), 2);
	CU_ASSERT_EQUAL(st.idx, 2);

	stack_free(&st);
	CU_ASSERT_EQUAL(st.idx, -1);
}

void test_stack_peek_pos(void)
{
	struct stack st = {0};
	
	CU_ASSERT(stack_init(&st));
	
	for (int i = 0; i < 3; i++) {
		CU_ASSERT(stack_push(&st, &i, sizeof(int)));
		CU_ASSERT_EQUAL(st.idx, i);
	}

	CU_ASSERT_EQUAL(*(int*)stack_peek_pos(&st, 0), 0);
	CU_ASSERT_EQUAL(*(int*)stack_peek_pos(&st, 1), 1);
	CU_ASSERT_EQUAL(*(int*)stack_peek_pos(&st, 2), 2);
	CU_ASSERT_EQUAL(st.idx, 2);

	stack_free(&st);
	CU_ASSERT_EQUAL(st.idx, -1);
}
