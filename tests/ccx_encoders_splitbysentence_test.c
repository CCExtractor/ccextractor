#include <check.h>

typedef int64_t LLONG;

#include "../src/lib_ccx/ccx_encoders_common.h"

// -------------------------------------
// STUBS
// -------------------------------------
struct encoder_ctx * context;
int string_index = 0;
char * STRINGS[] = {
	"Simple sentence.",
	"asdf",
	"asdf Hello!",
	"asdf Hello! It is me",
	"asdf Hello! It is me again",
	"Here is my coat.",
};


void freep(void * obj){
}
void fatal(int x, void * obj){
}
unsigned char * paraof_ocrtext(void * sub) {
	char * str = strdup(STRINGS[string_index]);
	return str;
}


// -------------------------------------
// preparations
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
START_TEST(test_sbs_one_sentece_no_reps)
{
	// Money *m = money_create(-1, "USD");

	// ck_assert_msg(m == NULL,
	// "NULL should be returned on attempt to create with "
	// "a negative amount"); 

	//struct cc_subtitle * reformat_cc_bitmap_through_sentence_buffer (struct cc_subtitle *sub, struct encoder_ctx *context);
	// sb_append_string();

	string_index = 0;

	struct cc_subtitle * sub1 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub1->type = CC_BITMAP;
	sub1->start_time = 1;
	sub1->end_time = 100;
	sub1->data = strdup(STRINGS[string_index]);
	sub1->nb_data = strlen(STRINGS[string_index]);

	struct cc_subtitle * out = reformat_cc_bitmap_through_sentence_buffer(sub1, context);

	ck_assert_ptr_ne(out, NULL);
	ck_assert_str_eq(out->data, STRINGS[string_index]);
	ck_assert_ptr_eq(out->next, NULL);
	ck_assert_ptr_eq(out->prev, NULL);
}
END_TEST


START_TEST(test_sbs_two_sentences)
{
	struct cc_subtitle * sub1 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub1->type = CC_BITMAP;
	sub1->start_time = 1;
	sub1->end_time = 100;
	sub1->data = strdup(STRINGS[0]);
	sub1->nb_data = strlen(STRINGS[0]);

	struct cc_subtitle * sub2 = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub2->type = CC_BITMAP;
	sub2->start_time = 101;
	sub2->end_time = 200;
	sub2->data = strdup(STRINGS[1]);
	sub2->nb_data = strlen(STRINGS[1]);

	struct cc_subtitle * out1 = reformat_cc_bitmap_through_sentence_buffer(sub1, context);

	ck_assert_ptr_ne(out1, NULL);
	ck_assert_str_eq(out1->data, STRINGS[0]);
	ck_assert_ptr_eq(out1->next, NULL);
	ck_assert_ptr_eq(out1->prev, NULL);

	struct cc_subtitle * out2 = reformat_cc_bitmap_through_sentence_buffer(sub2, context);

	ck_assert_ptr_eq(out2, NULL);
}
END_TEST


Suite * money_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Sentence Buffer");

	/* Core test case */
	tc_core = tcase_create("Break to sentences");

	tcase_add_checked_fixture(tc_core, setup, teardown);
	tcase_add_test(tc_core, test_sbs_one_sentece_no_reps);
	tcase_add_test(tc_core, test_sbs_two_sentences);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = money_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : 1;
}
