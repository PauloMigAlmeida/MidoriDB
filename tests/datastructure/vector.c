#include <tests/datastructure.h>
#include <datastructure/vector.h>

void test_vector_init(void)
{
	struct vector vec = {0};
	CU_ASSERT(vector_init(&vec));
	CU_ASSERT_EQUAL(vec.capacity, VECTOR_DEFAULT_INITAL_SIZE);
	CU_ASSERT_EQUAL(vec.len, 0);
	CU_ASSERT_PTR_NOT_NULL(vec.data);
	vector_free(&vec);

	/* bad input */
	CU_ASSERT_FALSE(vector_init(NULL));
}

void test_vector_push(void)
{
	struct vector vec = {0};
	char c = 'a';
	char *str = "test123456789012345678901234567890";
	size_t i;

	CU_ASSERT(vector_init(&vec));

	/* before vector increases its capacity */
	for (i = 0; i < VECTOR_DEFAULT_INITAL_SIZE / 2; i++) {
		CU_ASSERT(vector_push(&vec, &c, sizeof(c)));
		CU_ASSERT_EQUAL(vec.capacity, VECTOR_DEFAULT_INITAL_SIZE);
		CU_ASSERT_EQUAL(vec.len, i + 1);
	}

	/* force it grow in size */
	CU_ASSERT(vector_push(&vec, &c, sizeof(c)));
	i++;
	CU_ASSERT_EQUAL(vec.capacity, 27);
	CU_ASSERT_EQUAL(vec.len, i);

	/* force it grow by pushing a lot of data at once */
	CU_ASSERT(vector_push(&vec, &str, strlen(str)));
	i += strlen(str);
	CU_ASSERT_EQUAL(vec.capacity, 129);
	CU_ASSERT_EQUAL(vec.len, i);

	/* capacity should be affected in this case */
	CU_ASSERT(vector_push(&vec, &c, sizeof(c)));
	i++;
	CU_ASSERT_EQUAL(vec.capacity, 129);
	CU_ASSERT_EQUAL(vec.len, i);

	/* invalid len */
	CU_ASSERT_FALSE(vector_push(&vec, &c, 0));
	CU_ASSERT_EQUAL(vec.capacity, 129);
	CU_ASSERT_EQUAL(vec.len, i);

	/* invalid data */
	CU_ASSERT_FALSE(vector_push(&vec, NULL, sizeof(c)));
	CU_ASSERT_EQUAL(vec.capacity, 129);
	CU_ASSERT_EQUAL(vec.len, i);

	/* invalid vector reference */
	CU_ASSERT_FALSE(vector_push(NULL, &c, sizeof(c)));

	vector_free(&vec);
}

void test_vector_free(void)
{
	struct vector vec = {0};
	vector_init(&vec);
	vector_free(&vec);
	CU_ASSERT_EQUAL(vec.capacity, 0);
	CU_ASSERT_EQUAL(vec.len, 0);
	CU_ASSERT_PTR_NULL(vec.data);
}

