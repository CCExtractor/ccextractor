#include "lib_ccx.h"
#include "../ccextractor.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"

#ifdef PYTHON_API
void Asprintf(char **strp, const char *fmt, ...) {
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vasprintf(strp, fmt, ap);
	va_end(ap);

	if (ret == -1) {
		printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
		exit(CCX_COMMON_EXIT_BUG_BUG);
	}
}

void python_extract_g608_grid(unsigned h1, unsigned m1, unsigned s1, unsigned ms1,
	unsigned h2, unsigned m2, unsigned s2, unsigned ms2,
	char *buffer, int identifier, int srt_counter, int encoding) {
	/*
	 * identifier = 0 ---> adding start and end time
	 * identifier = 1 ---> subtitle
	 * identifier = 2 ---> color
	 * identifier = 3 ---> font
	 * identifier = 4 ---> end of frame
	 */
	char *output = NULL;
	char *start_time = NULL;
	char *end_time = NULL;

	switch (identifier) {
		case 0:
			Asprintf(&start_time, "%02d:%02d:%02d,%03d", h1, m1, s1, ms1);
			Asprintf(&end_time, "%02d:%02d:%02d,%03d", h2, m2, s2, ms2);
			Asprintf(&output, "srt_counter-%d\nstart_time-%s\t end_time-%s", srt_counter, start_time, end_time);

			free(start_time);
			free(end_time);
			break;
		case 1:
			Asprintf(&output, "text[%d]:%s", srt_counter, buffer);
			break;
		case 2:
			Asprintf(&output, "color[%d]:%s", srt_counter, buffer);
			break;
		case 3:
			Asprintf(&output, "font[%d]:%s", srt_counter, buffer);
			break;
		default:
			Asprintf(&output, "***END OF FRAME***");
			break;
	}

	py_callback(output, encoding);
	free(output);
}

int pass_cc_buffer_to_python(struct eia608_screen *data, struct encoder_ctx *context)
{
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int wrote_something = 0;
	char *timeline;

	millis_to_time(data->start_time, &h1, &m1, &s1, &ms1);
	millis_to_time(data->end_time - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.

	context->srt_counter++;
	Asprintf(&timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	encode_line(context, context->buffer, (unsigned char *)timeline);

	python_extract_g608_grid(h1, m1, s1, ms1, h2, m2, s2, ms2, context->buffer, 0, context->srt_counter, context->encoding);
	for (int i = 0; i < 15; i++)
	{
		int length = get_line_encoded(context, context->subline, i, data);
		python_extract_g608_grid(h1, m1, s1, ms1, h2, m2, s2, ms2, context->subline, 1, context->srt_counter, context->encoding);

		length = get_color_encoded(context, context->subline, i, data);
		python_extract_g608_grid(h1, m1, s1, ms1, h2, m2, s2, ms2, context->subline, 2, context->srt_counter, context->encoding);

		length = get_font_encoded(context, context->subline, i, data);
		python_extract_g608_grid(h1, m1, s1, ms1, h2, m2, s2, ms2, context->subline, 3, context->srt_counter, context->encoding);
		wrote_something = 1;
	}
	python_extract_g608_grid(h1, m1, s1, ms1, h2, m2, s2, ms2, context->subline, 4, context->srt_counter, context->encoding);
	return wrote_something;
}

#endif // PYTHON_API
