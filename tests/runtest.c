#include <check.h>

// TESTS:
#include "ccx_encoders_splitbysentence_suite.h"


int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = ccx_encoders_splitbysentence_suite();
	sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
