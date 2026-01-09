#include <check.h>
// FIXED: Added this header so we can use init_options()
#include "../src/lib_ccx/ccx_common_option.h"
#include "../src/lib_ccx/lib_ccx.h"

#include "ccx_encoders_splitbysentence_suite.h"

struct ccx_s_options ccx_options;
volatile int terminate_asap = 0;

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	// FIXED: Initialize options to defaults to prevent crashes
	init_options(&ccx_options);

	s = ccx_encoders_splitbysentence_suite();
	sr = srunner_create(s);

	// CK_NOFORK is fine for now
	srunner_set_fork_status(sr, CK_NOFORK);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}