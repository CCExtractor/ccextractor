#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#endif // !NK_IMPLEMENTATION

#include <stdio.h>
#include "preview.h"

int preview(struct nk_context *ctx, int x, int y, int width, int height, struct main_tab *main_settings)
{
	static int i;
	if (nk_begin(ctx, "Preview", nk_rect(x, y, width, height), NK_WINDOW_TITLE | NK_WINDOW_BACKGROUND))
	{
		nk_layout_row_dynamic(ctx, 20, 1);
		for (i = 0; i < main_settings->preview_string_count; i++)
			nk_label_wrap(ctx, main_settings->preview_string[i]);
	}
	nk_end(ctx);
	return !nk_window_is_closed(ctx, "Preview");
}
