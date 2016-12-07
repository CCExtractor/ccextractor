#include <check.h>
#include "ccx_encoders_splitbysentence_suite.h"

// -------------------------------------
// MOCKS
// -------------------------------------
typedef int64_t LLONG;
#include "../src/lib_ccx/ccx_encoders_common.h"

// -------------------------------------
// Private functions (for testing only)
// -------------------------------------
struct cc_subtitle * sbs_append_string(unsigned char * str, LLONG time_from, LLONG time_trim, struct encoder_ctx * context);

// -------------------------------------
// Test helpers
// -------------------------------------
struct encoder_ctx * context;

struct cc_subtitle * helper_create_sub(char * str, LLONG time_from, LLONG time_trim)
{
	struct cc_subtitle * sub = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub->type = CC_BITMAP;
	sub->start_time = 1;
	sub->end_time = 100;
	sub->data = strdup("asdf");
	sub->nb_data = strlen(sub->data);

	return sub;
}

// -------------------------------------
// MOCKS
// -------------------------------------

void freep(void * obj){
}
void fatal(int x, void * obj){
}

unsigned char * paraof_ocrtext(void * sub) {
	// this is OCR -> text converter.
	// now, in our test cases, we will pass TEXT instead of OCR.
	// and will return passed text as result

	return ((struct cc_subtitle *)sub)->data;
}

// -------------------------------------
// TEST preparations
// -------------------------------------
void setup(void)
{
	context = (struct encoder_ctx *)malloc(sizeof(struct encoder_ctx));
}

void teardown(void)
{
	free(context);
}

// -------------------------------------
// TESTS
// -------------------------------------
START_TEST(test_sbs_one_simple_sentence)
{
	// ck_assert_msg(m == NULL,
	// "NULL should be returned on attempt to create with "
	// "a negative amount");

	//struct cc_subtitle * reformat_cc_bitmap_through_sentence_buffer (struct cc_subtitle *sub, struct encoder_ctx *context);
	// sbs_append_string();

	struct cc_subtitle * sub1 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub1->type = CC_BITMAP;
	sub1->start_time = 1;
	sub1->end_time = 100;
	sub1->data = strdup("Simple sentence.");
	sub1->nb_data = strlen(sub1->data);

	struct cc_subtitle * out = reformat_cc_bitmap_through_sentence_buffer(sub1, context);

	ck_assert_ptr_ne(out, NULL);
	ck_assert_str_eq(out->data, "Simple sentence.");
	ck_assert_ptr_eq(out->next, NULL);
	ck_assert_ptr_eq(out->prev, NULL);
}
END_TEST


START_TEST(test_sbs_two_sentences_with_rep)
{
	struct cc_subtitle * sub1 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub1->type = CC_BITMAP;
	sub1->start_time = 1;
	sub1->end_time = 100;
	sub1->data = strdup("asdf");
	sub1->nb_data = strlen(sub1->data);

	struct cc_subtitle * out1 = reformat_cc_bitmap_through_sentence_buffer(sub1, context);
	ck_assert_ptr_eq(out1, NULL);

	// second sub:
	struct cc_subtitle * sub2 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub2->type = CC_BITMAP;
	sub2->start_time = 101;
	sub2->end_time = 200;
	sub2->data = strdup("asdf Hello.");
	sub2->nb_data = strlen(sub2->data);

	struct cc_subtitle * out2 = reformat_cc_bitmap_through_sentence_buffer(sub2, context);

	ck_assert_ptr_ne(out2, NULL);
	ck_assert_str_eq(out2->data, "asdf Hello.");
	ck_assert_ptr_eq(out2->next, NULL);
	ck_assert_ptr_eq(out2->prev, NULL);}
END_TEST


START_TEST(test_sbs_append_string_two_separate)
{
	unsigned char * test_strings[] = {
		"First string.",
		"Second string."
	};
	struct cc_subtitle * sub;
	unsigned char * str;

	// first string
	str = strdup(test_strings[0]);
	sub = NULL;
	sub = sbs_append_string(str, 1, 20, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, test_strings[0]);
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 20);

	// second string:
	str = strdup(test_strings[1]);
	sub = NULL;
	sub = sbs_append_string(str, 21, 40, context);

	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, test_strings[1]);
	ck_assert_int_eq(sub->start_time, 21);
	ck_assert_int_eq(sub->end_time, 40);
}
END_TEST

START_TEST(test_sbs_append_string_two_with_broken_sentence)
{
	// important !!
	// summary len == 32
	char * test_strings[] = {
		"First string",
		" ends here, deabbea."
	};
	struct cc_subtitle * sub;
	char * str;

	// first string
	str = strdup(test_strings[0]);
	sub = sbs_append_string(str, 1, 3, context);

	ck_assert_ptr_eq(sub, NULL);

	// second string:
	str = strdup(test_strings[1]);
	sub = sbs_append_string(str, 4, 5, context);

	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "First string ends here, deabbea.");
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 5);
}
END_TEST

START_TEST(test_sbs_append_string_two_intersecting)
{
	char * test_strings[] = {
		"First string",
		"First string ends here."
	};
	struct cc_subtitle * sub;
	char * str;

	// first string
	str = strdup(test_strings[0]);
	sub = sbs_append_string(str, 1, 20, context);

	ck_assert_ptr_eq(sub, NULL);
	free(sub);

	// second string:
	str = strdup(test_strings[1]);
	//printf("second string: [%s]\n", str);
	sub = sbs_append_string(str, 21, 40, context);

	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "First string ends here.");
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 40);
}
END_TEST


START_TEST(test_sbs_append_string_real_data_1)
{
	char * test_strings[] = {
		"First string",
		"First string ends here."
	};
	struct cc_subtitle * sub;
	char * str;

	// first string
	str = strdup(test_strings[0]);
	sub = sbs_append_string(str, 1, 20, context);

	ck_assert_ptr_eq(sub, NULL);
	free(sub);

	// second string:
	str = strdup(test_strings[1]);
	//printf("second string: [%s]\n", str);
	sub = sbs_append_string(str, 21, 40, context);

	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "First string ends here.");
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 40);
}
END_TEST



Suite * ccx_encoders_splitbysentence_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Sentence Buffer");

	/* Overall tests */
	tc_core = tcase_create("SB: Overall");

	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_sbs_one_simple_sentence);
	tcase_add_test(tc_core, test_sbs_two_sentences_with_rep);
	suite_add_tcase(s, tc_core);

	/**/
	TCase *tc_append_string;
	tc_append_string = tcase_create("SB: append_string");
	tcase_add_checked_fixture(tc_append_string, setup, teardown);

	tcase_add_test(tc_append_string, test_sbs_append_string_two_separate);
	tcase_add_test(tc_append_string, test_sbs_append_string_two_with_broken_sentence);
	tcase_add_test(tc_append_string, test_sbs_append_string_two_intersecting);
	tcase_add_test(tc_append_string, test_sbs_append_string_real_data_1);

	suite_add_tcase(s, tc_append_string);

	return s;
}
