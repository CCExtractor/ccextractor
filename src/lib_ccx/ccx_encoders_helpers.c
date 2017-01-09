#include "ccx_encoders_helpers.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

// userdefined rgb color
unsigned char usercolor_rgb[8] = "";

static int spell_builtin_added = 0; // so we don't do it twice
// Case arrays
char **spell_lower = NULL;
char **spell_correct = NULL;
int spell_words = 0;
int spell_capacity = 0;
// Some basic English words, so user-defined doesn't have to
// include the common stuff
static const char *spell_builtin[] =
{
	"I", "I'd", "I've", "I'd", "I'll",
	"January", "February", "March", "April", // May skipped intentionally
	"June", "July", "August", "September", "October", "November",
	"December", "Monday", "Tuesday", "Wednesday", "Thursday",
	"Friday", "Saturday", "Sunday", "Halloween", "United States",
	"Spain", "France", "Italy", "England",
	NULL
};

int string_cmp_function(const void *p1, const void *p2, void *arg)
{
	return strcasecmp(*(char**)p1, *(char**)p2);
}
int string_cmp(const void *p1, const void *p2)
{
	return string_cmp_function(p1, p2, NULL);
}

void correct_case_with_dictionary(int line_num, struct eia608_screen *data)
{
	char delim[64] = {
		' ', '\n', '\r', 0x89, 0x99,
		'!', '"', '#', '%', '&',
		'\'', '(', ')', ';', '<',
		'=', '>', '?', '[', '\\',
		']', '*', '+', ',', '-',
		'.', '/', ':', '^', '_',
		'{', '|', '}', '~', '\0' };

	char *line = strdup(((char*)data->characters[line_num]));
	char *oline = (char*)data->characters[line_num];
	char *c = strtok(line, delim);
	if (c == NULL)
	{
		free(line);
		return;
	}
	do
	{
		char **index = bsearch(&c, spell_lower, spell_words, sizeof(*spell_lower), string_cmp);

		if (index)
		{
			char *correct_c = *(spell_correct + (index - spell_lower));
			size_t len = strlen(correct_c);
			memcpy(oline + (c - line), correct_c, len);
		}
	} while ((c = strtok(NULL, delim)) != NULL);
	free(line);
}

void telx_correct_case(char *sub_line)
{
	char delim[64] = {
		' ', '\n', '\r', 0x89, 0x99,
		'!', '"', '#', '%', '&',
		'\'', '(', ')', ';', '<',
		'=', '>', '?', '[', '\\',
		']', '*', '+', ',', '-',
		'.', '/', ':', '^', '_',
		'{', '|', '}', '~', '\0' };

	char *line = strdup(((char*)sub_line));
	char *oline = (char*)sub_line;
	char *c = strtok(line, delim);
	if (c == NULL)
	{
		free(line);
		return;
	}
	do
	{
		char **index = bsearch(&c, spell_lower, spell_words, sizeof(*spell_lower), string_cmp);

		if (index)
		{
			char *correct_c = *(spell_correct + (index - spell_lower));
			size_t len = strlen(correct_c);
			memcpy(oline + (c - line), correct_c, len);
		}
	} while ((c = strtok(NULL, delim)) != NULL);
	free(line);
}

int is_all_caps(struct encoder_ctx *context, int line_num, struct eia608_screen *data)
{
	int saw_upper = 0, saw_lower = 0;

	for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; i++)
	{
		if (islower(data->characters[line_num][i]))
			saw_lower = 1;
		else if (isupper(data->characters[line_num][i]))
			saw_upper = 1;
	}
	return (saw_upper && !saw_lower); // 1 if we've seen upper and not lower, 0 otherwise
}

int clever_capitalize(struct encoder_ctx *context, int line_num, struct eia608_screen *data)
{
	// CFS: Tried doing to clever (see below) but some channels do all uppercase except for
	// notes for deaf people (such as "(narrator)" which messes things up.
		// First find out if we actually need to do it, don't mess with lines that come OK
		//int doit = is_all_caps(context, line_num, data);
	int doit = 1;

	for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; i++)
	{
		switch (data->characters[line_num][i])
		{
		case ' ':
		case 0x89: // This is a transparent space
		case '-':
			break;
		case '.': // Fallthrough
		case '?': // Fallthrough
		case '!':
		case ':':
			context->new_sentence = 1;
			break;
		default:
			if (doit)
			{
				if (context->new_sentence)
					data->characters[line_num][i] = cctoupper(data->characters[line_num][i]);
				else
					data->characters[line_num][i] = cctolower(data->characters[line_num][i]);
			}
			context->new_sentence = 0;
			break;
		}
	}
	return doit;
}

// Encodes a generic string. Note that since we use the encoders for closed caption
// data, text would have to be encoded as CCs... so using special characters here
// it's a bad idea.
unsigned encode_line(struct encoder_ctx *ctx, unsigned char *buffer, unsigned char *text)
{
	unsigned bytes = 0;
	while (*text)
	{
		switch (ctx->encoding)
		{
		case CCX_ENC_UTF_8:
		case CCX_ENC_LATIN_1:
			*buffer = *text;
			bytes++;
			buffer++;
			break;
		case CCX_ENC_UNICODE:
			*buffer = *text;
			*(buffer + 1) = 0;
			bytes += 2;
			buffer += 2;
			break;
		}
		text++;
	}
	*buffer = 0;
	return bytes;
}

unsigned get_decoder_line_encoded_for_gui(unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *line = data->characters[line_num];
	unsigned char *orig = buffer; // Keep for debugging
	int first = 0, last = 31;
	find_limit_characters(line, &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
	for (int i = first; i <= last; i++)
	{
		get_char_in_latin_1(buffer, line[i]);
		buffer++;
	}
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length

}

unsigned char *close_tag(struct encoder_ctx *ctx, unsigned char *buffer, char *tagstack, char tagtype, int *punderlined, int *pitalics, int *pchanged_font)
{
	for (int l = strlen(tagstack) - 1; l >= 0; l--)
	{
		char cur = tagstack[l];
		switch (cur)
		{
		case 'F':
			buffer += encode_line(ctx, buffer, (unsigned char *) "</font>");
			(*pchanged_font)--;
			break;
		case 'U':
			buffer += encode_line(ctx, buffer, (unsigned char *) "</u>");
			(*punderlined)--;
			break;
		case 'I':
			buffer += encode_line(ctx, buffer, (unsigned char *) "</i>");
			(*pitalics)--;
			break;
		}
		tagstack[l] = 0; // Remove from stack
		if (cur == tagtype) // We closed up to the required tag, done
			return buffer;
	}
	if (tagtype != 'A') // All
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Mismatched tags in encoding, this is a bug, please report");
	return buffer;
}

unsigned get_decoder_line_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	int col = COL_WHITE;
	int underlined = 0;
	int italics = 0;
	int changed_font = 0;
	char tagstack[128] = ""; // Keep track of opening/closing tags

	unsigned char *line = data->characters[line_num];
	unsigned char *orig = buffer; // Keep for debugging
	int first = 0, last = 31;
	if (ctx->trim_subs)
		find_limit_characters(line, &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
	for (int i = first; i <= last; i++)
	{
		// Handle color
		int its_col = data->colors[line_num][i];
		if (its_col != col  && !ctx->no_font_color &&
			!(col == COL_USERDEFINED && its_col == COL_WHITE)) // Don't replace user defined with white
		{
			if (changed_font)
				buffer = close_tag(ctx, buffer, tagstack, 'F', &underlined, &italics, &changed_font);

			// Add new font tag
			if ( MAX_COLOR > its_col)
				buffer += encode_line(ctx, buffer, (unsigned char*)color_text[its_col][1]);
			else
			{
				ccx_common_logging.log_ftn("WARNING:get_decoder_line_encoded:Invalid Color index Selected %d\n", its_col);
				its_col = COL_WHITE;
			}

			if (its_col == COL_USERDEFINED)
			{
				// The previous sentence doesn't copy the whole
				// <font> tag, just up to the quote before the color
				buffer += encode_line(ctx, buffer, (unsigned char*)usercolor_rgb);
				buffer += encode_line(ctx, buffer, (unsigned char*) "\">");
			}
			if (color_text[its_col][1][0]) // That means a <font> was added to the buffer
			{
				strcat(tagstack, "F");
				changed_font++;
			}
			col = its_col;
		}
		// Handle underlined
		int is_underlined = data->fonts[line_num][i] & FONT_UNDERLINED;
		if (is_underlined && underlined == 0 && !ctx->no_type_setting) // Open underline
		{
			buffer += encode_line(ctx, buffer, (unsigned char *) "<u>");
			strcat(tagstack, "U");
			underlined++;
		}
		if (is_underlined == 0 && underlined && !ctx->no_type_setting) // Close underline
		{
			buffer = close_tag(ctx, buffer, tagstack, 'U', &underlined, &italics, &changed_font);
		}
		// Handle italics
		int has_ita = data->fonts[line_num][i] & FONT_ITALICS;
		if (has_ita && italics == 0 && !ctx->no_type_setting) // Open italics
		{
			buffer += encode_line(ctx, buffer, (unsigned char *) "<i>");
			strcat(tagstack, "I");
			italics++;
		}
		if (has_ita == 0 && italics && !ctx->no_type_setting) // Close italics
		{
			buffer = close_tag(ctx, buffer, tagstack, 'I', &underlined, &italics, &changed_font);
		}
		int bytes = 0;
		switch (ctx->encoding)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(buffer, line[i]);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(buffer, line[i]);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(buffer, line[i]);
			bytes = 2;
			break;
		}
		buffer += bytes;
	}
	buffer = close_tag(ctx, buffer, tagstack, 'A', &underlined, &italics, &changed_font);
	if (underlined || italics || changed_font)
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Not all tags closed in encoding, this is a bug, please report.\n");
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length
}

void get_sentence_borders(int *first, int *last, int line_num, struct eia608_screen *data) {
	*first = 0;
	*last = 32;
	while (data->colors[line_num][*first] == COL_TRANSPARENT)
		(*first)++;
	while (data->colors[line_num][*last] == COL_TRANSPARENT)
		(*last)--;
}

/*void delete_all_lines_but_current(ccx_decoder_608_context *context, struct eia608_screen *data, int row)
{
for (int i=0;i<15;i++)
{
if (i!=row)
{
memset(data->characters[i], ' ', CCX_DECODER_608_SCREEN_WIDTH);
data->characters[i][CCX_DECODER_608_SCREEN_WIDTH] = 0;
memset(data->colors[i], context->settings.default_color, CCX_DECODER_608_SCREEN_WIDTH + 1);
memset(data->fonts[i], FONT_REGULAR, CCX_DECODER_608_SCREEN_WIDTH + 1);
data->row_used[i]=0;
}
}
}*/

/*void fprintf_encoded (struct encoder_ctx *ctx,FILE *fh, const char *string)
{
int used;
REQUEST_BUFFER_CAPACITY(ctx,strlen (string)*3);
used=encode_line (ctx->buffer,(unsigned char *) string);
fwrite (ctx->buffer,used,1,fh);
}*/

int add_word(const char *word)
{
	char *new_lower;
	char *new_correct;
	char **ptr_lower;
	char **ptr_correct;
	int i;
	if (spell_words == spell_capacity)
	{
		// Time to grow
		spell_capacity += 50;
		ptr_lower = (char **)realloc(spell_lower, sizeof (char *)*
			spell_capacity);
		ptr_correct = (char **)realloc(spell_correct, sizeof (char *)*
			spell_capacity);
	}
	else
	{
		ptr_lower = spell_lower;
		ptr_correct = spell_correct;
	}
	size_t len = strlen(word);
	new_lower = (char *)malloc(len + 1);
	new_correct = (char *)malloc(len + 1);
	if (ptr_lower == NULL || ptr_correct == NULL ||
		new_lower == NULL || new_correct == NULL)
	{
		spell_capacity = 0;
		for (i = 0; i < spell_words; i++)
		{
			freep(&spell_lower[spell_words]);
			freep(&spell_correct[spell_words]);
		}
		freep(&spell_lower);
		freep(&spell_correct);
		freep(&ptr_lower);
		freep(&ptr_correct);
		freep(&new_lower);
		freep(&new_correct);
		spell_words = 0;
		return -1;
	}
	else
	{
		spell_lower = ptr_lower;
		spell_correct = ptr_correct;
	}
	strcpy(new_correct, word);
	for (size_t i = 0; i<len; i++)
	{
		char c = new_correct[i];
		c = tolower(c); // TO-DO: Add Spanish characters
		new_lower[i] = c;
	}
	new_lower[len] = 0;
	spell_lower[spell_words] = new_lower;
	spell_correct[spell_words] = new_correct;
	spell_words++;
	return 0;
}


int add_built_in_words(void)
{
	if (!spell_builtin_added)
	{
		int i = 0;
		while (spell_builtin[i] != NULL)
		{
			if (add_word(spell_builtin[i]))
				return -1;
			i++;
		}
		spell_builtin_added = 1;
	}
	return 0;
}

/**
* @param base points to the start of the array
* @param nb   number of element in array
* @param size size of each element
* @param compar Comparison function, which is called with three argument
*               that point to the objects being compared and arg.
*		compare Funtion should return an integer less than, equal to,
*		or greater than zero if p1 is found, respectively, to be less than,
*		to match, or be greater than p2.
* @param arg argument passed as it is, to compare function
*/
void shell_sort(void *base, int nb, size_t size, int(*compar)(const void*p1, const void *p2, void*arg), void *arg)
{
	unsigned char *lbase = (unsigned char*)base;
	unsigned char *tmp = (unsigned char*)malloc(size);
	for (int gap = nb / 2; gap > 0; gap = gap / 2)
	{
		int p, j;
		for (p = gap; p < nb; p++)
		{
			memcpy(tmp, lbase + (p *size), size);
			for (j = p; j >= gap && (compar(tmp, lbase + ((j - gap) * size), arg) < 0); j -= gap)
			{
				memcpy(lbase + (j*size), lbase + ((j - gap) * size), size);
			}
			memcpy(lbase + (j *size), tmp, size);
		}
	}
	free(tmp);
}

void ccx_encoders_helpers_perform_shellsort_words(void)
{
	shell_sort(spell_lower, spell_words, sizeof(*spell_lower), string_cmp_function, NULL);
	shell_sort(spell_correct, spell_words, sizeof(*spell_correct), string_cmp_function, NULL);
}
