#include <tests/parser.h>

bool parser_init_suite(void)
{
	CU_pSuite suite = NULL;

	ADD_SUITE(suite, "parser");

	/* create stmt */
	ADD_UNITTEST(suite, test_syntax_parse);
	ADD_UNITTEST(suite, test_ast_build_tree);

	return false;
}

