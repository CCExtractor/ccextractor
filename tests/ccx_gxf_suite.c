#include <check.h>

#include "ccx_gxf_suite.h"
#include "../src/lib_ccx/ccx_common_common.h"
#include "../src/lib_ccx/ccx_demuxer.h"
#include "../src/lib_ccx/ccx_gxf.h"

int parse_ad_cdp(unsigned char *cdp, size_t len, struct demuxer_data *data);

static void discard_log(const char *fmt, ...)
{
	(void)fmt;
}

static void discard_debug(LLONG mask, const char *fmt, ...)
{
	(void)mask;
	(void)fmt;
}

static void set_test_logging(void)
{
	ccx_common_logging.log_ftn = discard_log;
	ccx_common_logging.debug_ftn = discard_debug;
}

START_TEST(test_cdp_rejects_cc_count_larger_than_packet)
{
	unsigned char cdp[] = {0x96, 0x69, 0x10, 0x40, 0x40, 0, 1, 0x72, 0x77, 0x38, 0x7b, 0x7d, 0x74, 0, 1, 0};
	struct demuxer_data data = {0};
	data.buffer = calloc(1, BUFSIZE);
	ck_assert_ptr_nonnull(data.buffer);
	set_test_logging();

	ck_assert_int_eq(parse_ad_cdp(cdp, sizeof(cdp), &data), CCX_EINVAL);
	ck_assert_int_eq(data.len, 0);
	free(data.buffer);
}
END_TEST

START_TEST(test_cdp_accepts_caption_data_that_fits)
{
	unsigned char cdp[] = {0x96, 0x69, 0x10, 0x40, 0x40, 0, 1, 0x72, 0x61, 1, 2, 3, 0x74, 0, 1, 0};
	struct demuxer_data data = {0};
	data.buffer = calloc(1, BUFSIZE);
	ck_assert_ptr_nonnull(data.buffer);
	set_test_logging();

	ck_assert_int_eq(parse_ad_cdp(cdp, sizeof(cdp), &data), CCX_OK);
	ck_assert_int_eq(data.len, 3);
	ck_assert_int_eq(data.buffer[0], 1);
	ck_assert_int_eq(data.buffer[1], 2);
	ck_assert_int_eq(data.buffer[2], 3);
	free(data.buffer);
}
END_TEST

START_TEST(test_cdp_rejects_caption_data_larger_than_destination)
{
	unsigned char cdp[] = {0x96, 0x69, 0x10, 0x40, 0x40, 0, 1, 0x72, 0x61, 1, 2, 3, 0x74, 0, 1, 0};
	struct demuxer_data data = {0};
	data.buffer = calloc(1, BUFSIZE);
	ck_assert_ptr_nonnull(data.buffer);
	data.len = BUFSIZE - 1;
	set_test_logging();

	ck_assert_int_eq(parse_ad_cdp(cdp, sizeof(cdp), &data), CCX_EINVAL);
	ck_assert_int_eq(data.len, BUFSIZE - 1);
	free(data.buffer);
}
END_TEST

Suite *ccx_gxf_suite(void)
{
	Suite *suite = suite_create("GXF");
	TCase *test_case = tcase_create("CDP bounds");

	tcase_add_test(test_case, test_cdp_rejects_cc_count_larger_than_packet);
	tcase_add_test(test_case, test_cdp_accepts_caption_data_that_fits);
	tcase_add_test(test_case, test_cdp_rejects_caption_data_larger_than_destination);
	suite_add_tcase(suite, test_case);
	return suite;
}
