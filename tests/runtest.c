#include <check.h>

#include "../src/lib_ccx/lib_ccx.h"
#include "../src/lib_ccx/ccx_common_option.h"

// TESTS:
#include "ccx_encoders_splitbysentence_suite.h"

struct ccx_s_options ccx_options;
volatile int terminate_asap = 0;

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = ccx_encoders_splitbysentence_suite();
	sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
