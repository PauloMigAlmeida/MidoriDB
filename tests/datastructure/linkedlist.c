#include "tests/datastructure.h"
#include "datastructure/linkedlist.h"

struct page_test {
        int data;
        struct list_head head;
};


void test_initialise_linkedlist(void) {
      struct page_test *x = malloc(sizeof(*x));
      list_head_init(&x->head);

      printf("hello world\n");
      free(x);
      CU_ASSERT(true);
}

