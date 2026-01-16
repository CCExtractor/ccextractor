#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_basename(char *filename)
{
	char *c;
	char *last_dot = NULL;
	char *last_slash = NULL;
	int len;
	char *basefilename;

	if (!filename)
		return NULL;

	len = strlen(filename);
	basefilename = (char *)malloc(len + 1);
	if (basefilename == NULL)
	{
		return NULL;
	}

	memcpy(basefilename, filename, len + 1);

	for (c = basefilename; *c; c++)
	{
		if (*c == '.')
			last_dot = c;
		else if (*c == '/' || *c == '\\')
			last_slash = c;
	}

	if (last_dot)
	{
		// If there is a slash, the dot must be AFTER the slash to be an extension
		if (last_slash)
		{
			if (last_dot > last_slash)
				*last_dot = 0;
		}
		else
		{
            if (last_dot != basefilename)
                *last_dot = 0;
		}
	}

	return basefilename;
}

int main() {
    char path[] = "./test_out/";
    char *base = get_basename(path);
    printf("Input: %s\nOutput: %s\n", path, base);
    free(base);
    return 0;
}
