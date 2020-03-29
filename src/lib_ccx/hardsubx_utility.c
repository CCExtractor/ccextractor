#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include "hardsubx.h"

int64_t convert_pts_to_ms(int64_t pts, AVRational time_base)
{
	return av_rescale_q(pts, time_base, AV_TIME_BASE_Q) / 1000;
}

int64_t convert_pts_to_ns(int64_t pts, AVRational time_base)
{
	return av_rescale_q(pts, time_base, AV_TIME_BASE_Q);
}

int64_t convert_pts_to_s(int64_t pts, AVRational time_base)
{
	return av_rescale_q(pts, time_base, AV_TIME_BASE_Q) / 1000000;
}

int edit_distance(char *word1, char *word2, int len1, int len2)
{
	// len2 <= len1
	if (len2 > len1)
	{
		int tmp = len1;
		len1 = len2;
		len2 = tmp;

		char *tmp_ptr = word1;
		word1 = word2;
		word2 = tmp_ptr;
	}

	int **matrix = (int **)malloc(2 * sizeof(int *));
	for (int i = 0; i < 2; i++)
		matrix[i] = (int *)malloc((len2 + 1) * sizeof(int));

	int i, delete, insert, substitute, minimum;
	for (i = 0; i <= len2; i++)
		matrix[0][i] = i;
	for (i = 1; i <= len1; i++)
	{
		int j;
		for (j = 0; j <= len2; j++)
			matrix[i % 2][j] = 0;
		matrix[i % 2][0] = i;

		char c1;
		c1 = word1[i - 1];
		for (j = 1; j <= len2; j++)
		{
			char c2;
			c2 = word2[j - 1];
			if (c1 == c2)
			{
				matrix[i % 2][j] = matrix[(i - 1) % 2][j - 1];
			}
			else
			{
				delete = matrix[(i - 1) % 2][j] + 1;
				insert = matrix[i % 2][j - 1] + 1;
				substitute = matrix[(i - 1) % 2][j - 1] + 1;
				minimum = delete;
				if (insert < minimum)
				{
					minimum = insert;
				}
				if (substitute < minimum)
				{
					minimum = substitute;
				}
				matrix[i % 2][j] = minimum;
			}
		}
	}

	int ret = matrix[len1 % 2][len2];
	for (int i = 0; i < 2; i++)
		free(matrix[i]);
	free(matrix);
	return ret;
}

int is_valid_trailing_char(char c)
{
	char *prune_text = ",~`'-_+=‘:;” \n";
	printf("%c ", c);
	int ret = 1;
	size_t len = strlen(prune_text);

	for (int i = 0; i < len; i++)
	{
		if (prune_text[i] == c)
		{
			ret = -1;
			break;
		}
	}
	printf("%d\n", ret);
	return ret;
}

char *prune_string(char *s)
{
	size_t size;
	char *end;

	size = strlen(s);

	if (!size)
		return s;

	end = s + size - 1;
	while (end >= s && is_valid_trailing_char(*end) == -1)
		end--;
	*(end + 1) = '\0';

	while (*s && is_valid_trailing_char(*end) == -1)
		s++;
	printf("%s\n", s);
	return s;
}

#endif
