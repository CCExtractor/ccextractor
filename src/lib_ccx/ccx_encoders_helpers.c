#include "ccx_encoders_helpers.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_decoders_common.h"

#include <assert.h>

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

// userdefined rgb color
unsigned char usercolor_rgb[8] = "";

struct word_list
{
	char **words;
	size_t len;
	size_t capacity;
};

struct word_list capitalization_list = {
    .words = NULL,
    .len = 0,
    .capacity = 0,
};

struct word_list profane = {
    .words = NULL,
    .len = 0,
    .capacity = 0,
};

// Some basic English words, so user-defined doesn't have to
// include the common stuff
const char *capitalized_builtin[] =
    {
	"I", "I'd", "I've", "I'd", "I'll",
	"January", "February", "March", "April", // May skipped intentionally
	"June", "July", "August", "September", "October", "November",
	"December", "Monday", "Tuesday", "Wednesday", "Thursday",
	"Friday", "Saturday", "Sunday", "Halloween", "United States",
	"Spain", "France", "Italy", "England",
	NULL};

const char *profane_builtin[] =
    {
	"arse",
	"ass",
	"asshole",
	"bastard",
	"bitch",
	"bollocks",
	"child-fucker",
	"crap",
	"cunt",
	"damn",
	"frigger",
	"fuck",
	"goddamn",
	"godsdamn",
	"hell",
	"holy",
	"horseshit",
	"motherfucker",
	"nigga",
	"nigger",
	"prick",
	"shit",
	"shitass",
	"slut",
	"twat",
	NULL};

int string_cmp_function(const void *p1, const void *p2, void *arg)
{
	return strcasecmp(*(char **)p1, *(char **)p2);
}

int string_cmp(const void *p1, const void *p2)
{
	return string_cmp_function(p1, p2, NULL);
}

void capitalize_word(size_t index, unsigned char *word)
{
	memcpy(word, capitalization_list.words[index], strlen(capitalization_list.words[index]));
}

void censor_word(size_t index, unsigned char *word)
{
	memset(word, 0x98, strlen(profane.words[index])); // 0x98 is the asterisk in EIA-608
}

void call_function_if_match(unsigned char *line, struct word_list *list, void (*modification)(size_t, unsigned char *))
{
	unsigned char delim[64] = {
	    ' ', '\n', '\r', 0x89, 0x99,
	    '!', '"', '#', '%', '&',
	    '\'', '(', ')', ';', '<',
	    '=', '>', '?', '[', '\\',
	    ']', '*', '+', ',', '-',
	    '.', '/', ':', '^', '_',
	    '{', '|', '}', '~', '\0'};

	unsigned char *line_token = strdup(line);
	unsigned char *c = strtok(line_token, delim);

	if (c != NULL)
	{
		do
		{
			unsigned char **index = bsearch(&c, list->words, list->len, sizeof(*list->words), string_cmp);

			if (index)
			{
				modification(index - (unsigned char **)list->words, line + (c - line_token));
			}
		} while ((c = strtok(NULL, delim)) != NULL);
	}
	free(line_token);
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
	    '{', '|', '}', '~', '\0'};

	char *line = strdup(((char *)sub_line));
	char *oline = (char *)sub_line;
	char *c = strtok(line, delim);
	if (c == NULL)
	{
		free(line);
		return;
	}
	do
	{
		char **index = bsearch(&c, capitalization_list.words, capitalization_list.len, sizeof(*capitalization_list.words), string_cmp);

		if (index)
		{
			char *correct_c = capitalization_list.words[index - capitalization_list.words];
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

int clever_capitalize(struct encoder_ctx *context, unsigned char *line, unsigned int length)
{
	// CFS: Tried doing to clever (see below) but some channels do all uppercase except for
	// notes for deaf people (such as "(narrator)" which messes things up.
	// First find out if we actually need to do it, don't mess with lines that come OK
	// int doit = is_all_caps(context, line_num, data);
	int doit = 1;

	for (int i = 0; i < length; i++)
	{
		switch (line[i])
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
						line[i] = cctoupper(line[i]);
					else
						line[i] = cctolower(line[i]);
				}
				context->new_sentence = 0;
				break;
		}
	}
	return doit;
}

// Encodes a generic string. Note that since we use the encoders for closed caption
// data, text would have to be encoded as CCs... so using special characters here
// is a bad idea.
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
			case CCX_ENC_ASCII: // Consider to remove ASCII encoding or write some code here
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
				buffer += encode_line(ctx, buffer, (unsigned char *)"</font>");
				(*pchanged_font)--;
				break;
			case 'U':
				buffer += encode_line(ctx, buffer, (unsigned char *)"</u>");
				(*punderlined)--;
				break;
			case 'I':
				buffer += encode_line(ctx, buffer, (unsigned char *)"</i>");
				(*pitalics)--;
				break;
		}
		tagstack[l] = 0;    // Remove from stack
		if (cur == tagtype) // We closed up to the required tag, done
			return buffer;
	}
	if (tagtype != 'A') // All
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Mismatched tags in encoding, this is a bug, please report");
	return buffer;
}

unsigned get_decoder_line_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	enum ccx_decoder_608_color_code color = COL_WHITE;
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
		enum ccx_decoder_608_color_code its_color = data->colors[line_num][i];
		// Check if the colour has changed
		if (its_color != color && !ctx->no_font_color &&
		    !(color == COL_USERDEFINED && its_color == COL_WHITE)) // Don't replace user defined with white
		{
			if (changed_font)
				buffer = close_tag(ctx, buffer, tagstack, 'F', &underlined, &italics, &changed_font);

			// Add new font tag
			if (MAX_COLOR > its_color)
				buffer += encode_line(ctx, buffer, (unsigned char *)color_text[its_color][1]);
			else
			{
				ccx_common_logging.log_ftn("WARNING:get_decoder_line_encoded:Invalid Color index Selected %d\n", its_color);
				its_color = COL_WHITE;
			}

			if (its_color == COL_USERDEFINED)
			{
				// The previous sentence doesn't copy the whole
				// <font> tag, just up to the quote before the color
				buffer += encode_line(ctx, buffer, (unsigned char *)usercolor_rgb);
				buffer += encode_line(ctx, buffer, (unsigned char *)"\">");
			}
			if (color_text[its_color][1][0]) // That means a <font> was added to the buffer
			{
				strcat(tagstack, "F");
				changed_font++;
			}
			color = its_color;
		}
		// Handle underlined
		int is_underlined = data->fonts[line_num][i] & FONT_UNDERLINED;
		if (is_underlined && underlined == 0 && !ctx->no_type_setting) // Open underline
		{
			buffer += encode_line(ctx, buffer, (unsigned char *)"<u>");
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
			buffer += encode_line(ctx, buffer, (unsigned char *)"<i>");
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
			case CCX_ENC_ASCII: // Consider to remove ASCII encoding or write get_char_in_ascii(...)
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

void get_sentence_borders(int *first, int *last, int line_num, struct eia608_screen *data)
{
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

// Add the word the `struct word_list` list
int add_word(struct word_list *list, const char *word)
{
	if (list->len == list->capacity)
	{
		list->capacity += 50;
		if ((list->words = realloc(list->words, list->capacity * sizeof(char *))) == NULL)
		{
			return -1;
		}
	}

	size_t word_len = strlen(word);
	if ((list->words[list->len] = malloc(word_len + 1)) == NULL)
	{
		return -1;
	}

	strcpy(list->words[list->len++], word);
	return word_len;
}

int add_builtin_words(const char *builtin[], struct word_list *list)
{
	int i = 0;
	while (builtin[i] != NULL)
	{
		if (add_word(list, builtin[i++]) == -1)
			return -1;
	}
	return 0;
}

void correct_spelling_and_censor_words(struct encoder_ctx *context, unsigned char *line, unsigned int length)
{
	if (context->sentence_cap)
	{
		if (clever_capitalize(context, line, length))
			call_function_if_match(line, &capitalization_list, capitalize_word);
	}

	if (context->filter_profanity)
	{
		call_function_if_match(line, &profane, censor_word);
	}
}

/**
 * @param base points to the start of the array
 * @param nb   number of element in array
 * @param size size of each element
 * @param compar Comparison function, which is called with three argument
 *               that point to the objects being compared and arg.
 *		compare Function should return an integer less than, equal to,
 *		or greater than zero if p1 is found, respectively, to be less than,
 *		to match, or be greater than p2.
 * @param arg argument passed as it is, to compare function
 */
void shell_sort(void *base, int nb, size_t size, int (*compar)(const void *p1, const void *p2, void *arg), void *arg)
{
	unsigned char *lbase = (unsigned char *)base;
	unsigned char *tmp = (unsigned char *)malloc(size);
	for (int gap = nb / 2; gap > 0; gap = gap / 2)
	{
		int p, j;
		for (p = gap; p < nb; p++)
		{
			memcpy(tmp, lbase + (p * size), size);
			for (j = p; j >= gap && (compar(tmp, lbase + ((j - gap) * size), arg) < 0); j -= gap)
			{
				memcpy(lbase + (j * size), lbase + ((j - gap) * size), size);
			}
			memcpy(lbase + (j * size), tmp, size);
		}
	}
	free(tmp);
}

void ccx_encoders_helpers_perform_shellsort_words(void)
{
	shell_sort(capitalization_list.words, capitalization_list.len, sizeof(*capitalization_list.words), string_cmp_function, NULL);
	shell_sort(profane.words, profane.len, sizeof(*profane.words), string_cmp_function, NULL);
}
