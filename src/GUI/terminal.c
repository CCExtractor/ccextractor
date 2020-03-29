#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#endif // !NK_IMPLEMENTATION

static int
terminal(struct nk_context *ctx, int x, int y, int width, int height, char *command)
{
	if (nk_begin(ctx, "Terminal", nk_rect(x, y, width, height),
		     NK_WINDOW_TITLE | NK_WINDOW_BACKGROUND))
	{
		nk_layout_row_dynamic(ctx, 60, 1);
		nk_label_wrap(ctx, command);
	}
	nk_end(ctx);
	return !nk_window_is_closed(ctx, "Terminal");
}
