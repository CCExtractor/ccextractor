#include <check.h>
#include "ccx_encoders_splitbysentence_suite.h"

// -------------------------------------
// MOCKS
// -------------------------------------
typedef int64_t LLONG;
#include "../src/lib_ccx/ccx_encoders_common.h"

// -------------------------------------
// Private SBS-functions (for testing only)
// -------------------------------------
struct cc_subtitle * sbs_append_string(unsigned char * str, LLONG time_from, LLONG time_trim, struct encoder_ctx * context);

// -------------------------------------
// Helpers
// -------------------------------------
struct cc_subtitle * helper_create_sub(char * str, LLONG time_from, LLONG time_trim) {
	struct cc_subtitle * sub = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	sub->type = CC_BITMAP;
	sub->start_time = 1;
	sub->end_time = 100;
	sub->data = strdup(str);
	sub->nb_data = strlen(sub->data);

	return sub;
}

struct cc_subtitle * helper_sbs_append_string(char * str, LLONG time_from, LLONG time_trim, struct encoder_ctx * context) {
	char * str1;
	struct cc_subtitle * sub;

	str1 = strdup(str);
	sub = sbs_append_string(str1, time_from, time_trim, context);
	free(str1);
	return sub;
}

struct cc_subtitle * helper_sbs_append_sub_from_file(FILE * fd, struct encoder_ctx * context) {
	// TODO : I am not sure about correctness of this line,
	// but I just want to test the code:
	char localbuf[2000];
	char * str;
	LLONG time_from, time_trim;

	if (feof(fd)) {
		return NULL;
	}

	if ( 0 >= fscanf(fd, "%ld %ld", &time_from, &time_trim)) {
		return NULL;
	}

	fgets(localbuf, 2000, fd);

	// skip leading spaces
	str = localbuf;
	for (; isspace(*str); str++){
	}

	// replace LITERAL "\n" with a newline char
	size_t i, j, L;
	L = strlen(str);
	for (i=0, j=0; i < L; i++, j++) {

		if (i + 1 < L) {
			if (str[i] == '\\' && str[i+1] == 'n') {
				i++;
				str[i] = '\n';
			}
		}
		str[j] = str[i];
	}

	// remove trailing newline:
	for(; j>0 && str[j] == '\n'; j--) {
		str[j] = 0;
	}
	//str1 = strdup(str);
	struct cc_subtitle * sub;
	sub = sbs_append_string(str, time_from, time_trim, context);
	//free(str1);
	return sub;
}

// -------------------------------------
// MOCKS
// -------------------------------------
struct encoder_ctx * context;

// void freep(void * obj){
// }
// void fatal(int x, void * obj){
// }

// unsigned char * paraof_ocrtext(void * sub) {
// 	// this is OCR -> text converter.
// 	// now, in our test cases, we will pass TEXT instead of OCR.
// 	// and will return passed text as result

// 	return ((struct cc_subtitle *)sub)->data;
// }

// -------------------------------------
// TEST preparations
// -------------------------------------
void setup(void)
{
	context = (struct encoder_ctx *)malloc(sizeof(struct encoder_ctx));
	context->sbs_buffer = NULL;
	context->sbs_capacity = 0;
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
	struct cc_subtitle * sub = helper_create_sub("Simple sentence.", 1, 100);
	struct cc_subtitle * out = reformat_cc_bitmap_through_sentence_buffer(sub, context);

	ck_assert_ptr_ne(out, NULL);
	ck_assert_str_eq(out->data, "Simple sentence.");
	ck_assert_ptr_eq(out->next, NULL);
	ck_assert_ptr_eq(out->prev, NULL);
}
END_TEST


START_TEST(test_sbs_two_sentences_with_rep)
{
	struct cc_subtitle * sub1 = helper_create_sub("asdf", 1, 100);
	struct cc_subtitle * out1 = reformat_cc_bitmap_through_sentence_buffer(sub1, context);
	ck_assert_ptr_eq(out1, NULL);

	// second sub:
	struct cc_subtitle * sub2 = helper_create_sub("asdf Hello.", 101, 200);
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
	sub = sbs_append_string(str, 21, 40, context);

	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "First string ends here.");
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 40);
}
END_TEST



START_TEST(test_sbs_append_string_00)
{
	FILE * fsample;
	int skip;

	fsample = fopen("samples/sbs_append_string_00", "r");
	struct cc_subtitle * sub;

	skip = 2;
	while (skip-- > 0) {
		sub = helper_sbs_append_sub_from_file(fsample, context);
		ck_assert_ptr_eq(sub, NULL);
	}

	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "in all these different environments, just \
doing what turkeys want to do‘ Let's get the sport now with an update from \
Sky Sports News HQ.");
	ck_assert_int_eq(sub->start_time, 1);
	ck_assert_int_eq(sub->end_time, 3467);

	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_eq(sub, NULL);

	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "Malky Mackay believes he deserves a second \
chance after being appointed the Scottish FA's new Performance Director.");
	ck_assert_int_eq(sub->start_time, 3468);
	ck_assert_int_eq(sub->end_time, 16361);

	skip = 10;
	while (skip-- > 0) {
		sub = helper_sbs_append_sub_from_file(fsample, context);
		ck_assert_ptr_eq(sub, NULL);
	}

	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "Mackay was sacked by Cardiff in 2013 after it \
emerged he sent racist There has been some opposition When I said at the time, \
I deeply to his appointment but he's asked regret.");
	ck_assert_int_eq(sub->start_time, 16362);
	ck_assert_int_eq(sub->end_time, 38924);

	fclose(fsample);
}
END_TEST


START_TEST(test_sbs_append_string_01)
{
	FILE * fsample;
	int skip;

	fsample = fopen("samples/sbs_append_string_01", "r");
	struct cc_subtitle * sub;

	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_eq(sub, NULL);

	// 2
	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "Oleon costs.");

	// 3
	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "buried in the annex, 95 Oleon costs.");
	ck_assert_int_eq(sub->start_time, 190);    // = <sub start>
	ck_assert_int_eq(sub->end_time, 783);      // = <sub start>  +  <available time,889-190=699 > * <sentence alphanum, 28>  /  <sub alphanum, 33>
	ck_assert_ptr_eq(sub->next, NULL);

	skip = 5;
	while (skip-- > 0) {
		sub = helper_sbs_append_sub_from_file(fsample, context);
		ck_assert_ptr_eq(sub, NULL);
	}

	// 13
	sub = helper_sbs_append_sub_from_file(fsample, context);
	ck_assert_ptr_ne(sub, NULL);
	ck_assert_str_eq(sub->data, "Didn't want to acknowledge the pressures on hospitals, schools and infrastructure.");
	ck_assert_int_eq(sub->start_time, 784);
	ck_assert_int_eq(sub->end_time, 5159);
	ck_assert_ptr_eq(sub->next, NULL);

	skip = 2;
	while (skip-- > 0) {
		sub = helper_sbs_append_sub_from_file(fsample, context);
		ck_assert_ptr_eq(sub, NULL);
	}

	// ck_assert_int_eq(sub->start_time, 1);
	// ck_assert_int_eq(sub->end_time, 40);
	fclose(fsample);
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
	tcase_add_test(tc_append_string, test_sbs_append_string_00);
	tcase_add_test(tc_append_string, test_sbs_append_string_01);

	suite_add_tcase(s, tc_append_string);

	return s;
}
