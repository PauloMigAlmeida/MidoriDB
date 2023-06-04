#include <tests/lib.h>
#include <lib/regex.h>

void test_regex_ext_match_grp(void)
{
	char *str = "CREATE 1 2 TEST";
	struct stack reg_pars = {0};

	CU_ASSERT(stack_init(&reg_pars));
	CU_ASSERT(regex_ext_match_grp(str, "CREATE ([0-9]+) ([0-9]+) ([A-Za-z][A-Za-z0-9_]*)", &reg_pars));
	CU_ASSERT_STRING_EQUAL((char* )stack_peek_pos(&reg_pars, 0), "1");
	CU_ASSERT_STRING_EQUAL((char* )stack_peek_pos(&reg_pars, 1), "2");
	CU_ASSERT_STRING_EQUAL((char* )stack_peek_pos(&reg_pars, 2), "TEST");

	stack_free(&reg_pars);

}
