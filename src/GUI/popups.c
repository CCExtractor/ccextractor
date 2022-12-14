#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#include <stdio.h>
#include <string.h>
#endif // !NK_IMPLEMENTATION
#include "tabs.h"
#include "popups.h"

void setup_network_settings(struct network_popup *network_settings)
{
	network_settings->show_network_settings = nk_false;
	network_settings->save_network_settings = nk_false;
	strcpy(network_settings->udp_ipv4, "None");
	network_settings->udp_ipv4_len = strlen(network_settings->udp_ipv4);
	strcpy(network_settings->tcp_pass, "None");
	network_settings->tcp_pass_len = strlen(network_settings->tcp_pass);
	strcpy(network_settings->tcp_desc, "None");
	network_settings->tcp_desc_len = strlen(network_settings->tcp_desc);
	strcpy(network_settings->send_port, "None");
	network_settings->send_port_len = strlen(network_settings->send_port);
	strcpy(network_settings->send_host, "None");
	network_settings->send_host_len = strlen(network_settings->send_host);
}

void draw_network_popup(struct nk_context *ctx, struct network_popup *network_settings)
{
	const float save_ok_ratio[] = {0.8f, 0.1f, 0.1f};
	const float udp_tcp_ratio[] = {0.45f, 0.1f, 0.45f};
	static char udp_ipv4_buffer[30];
	static int udp_ipv4_len[30];
	static char tcp_pass_buf[30];
	static int tcp_pass_len[30];
	static char tcp_desc_buf[30];
	static int tcp_desc_len[30];
	static char send_port_buf[30];
	static int send_port_len[30];
	static char send_host_buf[30];
	static int send_host_len[30];
	const char network_attr[][30] = {"-udp port:", "-udp [host:]port:", "-sendto host[:port]:", "-tcp port:", "-tcppassword password:", "-tcpdesc description:"};
	static struct nk_rect s = {20, 30, 480, 500};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Network Settings", NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR, s))
	{
		nk_layout_row_dynamic(ctx, 220, 1);
		if (nk_group_begin(ctx, "Receive", NK_WINDOW_TITLE))
		{
			nk_layout_row(ctx, NK_DYNAMIC, 21, 3, udp_tcp_ratio);
			nk_spacing(ctx, 1);
			nk_label(ctx, "UDP:", NK_TEXT_CENTERED);
			nk_layout_row_static(ctx, 20, 200, 2);
			nk_label(ctx, "Hostname/IPv4 Address:", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_SIMPLE, network_settings->udp_ipv4, &network_settings->udp_ipv4_len, 50, nk_filter_default);

			nk_layout_row(ctx, NK_DYNAMIC, 21, 3, udp_tcp_ratio);
			nk_spacing(ctx, 1);
			nk_label(ctx, "TCP:", NK_TEXT_CENTERED);
			nk_layout_row_static(ctx, 20, 200, 2);
			nk_label(ctx, "Password:", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_SIMPLE, network_settings->tcp_pass, &network_settings->tcp_pass_len, 25, nk_filter_default);
			nk_layout_row_static(ctx, 20, 200, 2);
			nk_label(ctx, "Description:", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_SIMPLE, network_settings->tcp_desc, &network_settings->tcp_desc_len, 25, nk_filter_default);

			nk_group_end(ctx);
		}

		nk_layout_row_dynamic(ctx, 200, 1);
		if (nk_group_begin(ctx, "Send", NK_WINDOW_TITLE))
		{
			nk_layout_row(ctx, NK_DYNAMIC, 21, 3, udp_tcp_ratio);
			nk_spacing(ctx, 1);
			nk_label(ctx, "Send to:", NK_TEXT_CENTERED);
			nk_layout_row_static(ctx, 20, 200, 2);
			nk_label(ctx, "Port:", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_SIMPLE, network_settings->send_port, &network_settings->send_port_len, 25, nk_filter_default);
			nk_layout_row_static(ctx, 20, 200, 2);
			nk_label(ctx, "Host:", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_SIMPLE, network_settings->send_host, &network_settings->send_host_len, 25, nk_filter_default);

			nk_group_end(ctx);
		}

		/*nk_layout_row_static(ctx, 20, 200, 2);
		nk_label(ctx, network_attr[5], NK_TEXT_LEFT);
		nk_edit_string(ctx, NK_EDIT_SIMPLE, text_buffer[5], &text_len[5], 50, nk_filter_default);*/

		// OK Button
		nk_layout_row(ctx, NK_DYNAMIC, 27, 3, save_ok_ratio);
		nk_spacing(ctx, 1);
		if (nk_button_label(ctx, "Save"))
		{
			network_settings->save_network_settings = nk_true;
			network_settings->show_network_settings = nk_false;
			nk_popup_close(ctx);
		}
		if (nk_button_label(ctx, "OK"))
		{
			network_settings->save_network_settings = nk_false;
			network_settings->show_network_settings = nk_false;
			nk_popup_close(ctx);
		}

		nk_popup_end(ctx);
	}
	else
		network_settings->show_network_settings = nk_false;
}

void draw_getting_started_popup(struct nk_context *ctx, int *show_getting_started)
{
	static struct nk_rect s = {20, 30, 480, 500};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Getting Started", NK_WINDOW_CLOSABLE, s))
	{
		nk_layout_row_dynamic(ctx, 80, 1);
		nk_label_wrap(ctx, "Getting Started information about CCX will come here! This popup will be populated at the end.");
		nk_popup_end(ctx);
	}
	else
		*show_getting_started = nk_false;
}

void draw_about_ccx_popup(struct nk_context *ctx, int *show_about_ccx, struct nk_user_font *droid_big, struct nk_user_font *droid_head)
{
	const float ccx_ratio[] = {0.3f, 0.4f, 0.3f};
	const float ok_ratio[] = {0.9f, 0.1f};
	static struct nk_rect s = {20, 30, 480, 500};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About CCExtractor", NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR, s))
	{
		nk_style_push_font(ctx, droid_big);
		nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ccx_ratio);
		nk_spacing(ctx, 1);
		nk_label_wrap(ctx, "About CCExtractor" /*, NK_TEXT_LEFT*/);
		nk_style_pop_font(ctx);

		nk_layout_row_dynamic(ctx, 390, 1);
		if (nk_group_begin(ctx, "About CCExtractor", NK_WINDOW_BACKGROUND))
		{
			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "What's CCExtractor?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 65, 1);
			nk_label_wrap(ctx, "A tool that analyzes video files and produces independent subtitle files from the closed captions data. CCExtractor is portable, small, and very fast. It works in Linux, Windows, and OSX.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "What kind of closed captions does CCExtractor support?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 47, 1);
			nk_label_wrap(ctx, "American TV captions (CEA-608 is well supported, and CEA-708 is starting to look good) and Teletext based European subtitles.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "How easy is it to use CCExtractor?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 30, 1);
			nk_label_wrap(ctx, "Very. Just tell it what file to process and it does everything for you.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "CCExtractor integration with other tools");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 147, 1);
			nk_label_wrap(ctx, "It is possible to integrate CCExtractor in a larger process. A couple of tools already call CCExtractor as part their video process - this way they get subtitle support for free. Starting in 0.52, CCExtractor is very front - end friendly.Front - ends can easily get real - time status information.The GUI source code is provided and can be used for reference. Any tool, commercial or not, is specifically allowed to use CCExtractor for any use the authors seem fit. So if your favourite video tools still lacks captioning tool, feel free to send the authors here.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 50, 1);
			nk_label_wrap(ctx, "What's the point of generating separate files for subtitles, if they are already in the source file?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 367, 1);
			nk_label_wrap(ctx, "There are several reasons to have subtitles separated from the video file, including: - Closed captions never survive MPEG processing. If you take a MPEG file and encode it to any format (such as divx), your result file will not have closed captions. This means that if you want to keep the subtitles, you need to keep the original file. This is hardly practical if you are archiving HDTV shows for example. - Subtitles files are small - so small (around 250 Kb for a movie) that you can quickly download them, or email them, etc, in case you have a recording without subtitles. - Subtitles files are indexable: You can have a database with all your subtitles if you want (there are many available), so you can search the dialogs. - Subtitles files are a de-facto standard: Almost every player can use them. In fact, many setbox players accept subtitles files in .srt format - so you can have subtitles in your divx movies and not just in your original DVDs. - Closed captions are stored in many different formats by capture cards. Upgrading to a new card, if it comes with a new player, may mean that you can't use your previously recorded closed captions, even if the audio/video are fine. - Closed captions require a closed caption decoder. All US TV have one (it's a legal requirement), but no European TV does, since there are not closed captions in Europe (teletext is used instead). Basically this means that if you buy a DVD in the US which has closed captions but no DVD subtitles, you are out of luck. This is a problem with many (most) old TV shows DVDs, which only come with closed captions. DVD producers don't bother doing DVD subs, since it's another way to segment the market, same as with DVD regions. ");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "How I do use subtitles once they are in a separate file?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 80, 1);
			nk_label_wrap(ctx, "CCExtractor generates files in the two most common formats: .srt (SubRip) and .smi (which is a Microsoft standard). Most players support at least .srt natively. You just need to name the .srt file as the file you want to play it with, for example sample.avi and sample.srt.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "What kind of files can I extract closed captions from?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "CCExtractor currently handles:");
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- DVDs.");
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- Most HDTV captures(where you save the Transport Stream).");
			nk_layout_row_dynamic(ctx, 52, 1);
			nk_label_wrap(ctx, "- Captures where captions are recorded in bttv format.The number of cards that use this card is huge.My test samples came from a Hauppage PVR - 250. You can check the complete list here:");
			nk_layout_row_dynamic(ctx, 40, 1);
			nk_label_colored_wrap(ctx, "http://linuxtv.org/hg/v4l-dvb/file/tip/linux/Documentation/video4linux/CARDLIST.bttv", nk_rgb(61, 117, 206));
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- DVR - MS(microsoft digital video recording).");
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- Tivo files");
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- ReplayTV files");
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label_wrap(ctx, "- Dish Network files");
			nk_layout_row_dynamic(ctx, 80, 1);
			nk_label_wrap(ctx, "Usually, if you record a TV show with your capture card and CCExtractor produces the expected result, it will work for your all recordings.If it doesn't, which means that your card uses a format CCExtractor can't handle, please contact me and we'll try to make it work.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "Can I edit the subtitles? ");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 43, 1);
			nk_label_wrap(ctx, ".srt files are just text files, with time information (when subtitles are supposed to be shown and for how long) and some basic formatting (use italics, bold, etc). So you can edit them with any text editor. If you need to do serious editing (such as adjusting timing), you can use subtitle editing tools - there are many available.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "Can CCExtractor generate other subtitles formats?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "At this time, CCExtractor can generate .srt, .smi and raw and bin files.");

			nk_style_push_font(ctx, droid_head);
			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "How I can contact the author?");
			nk_style_pop_font(ctx);

			nk_layout_row_dynamic(ctx, 23, 1);
			nk_label_wrap(ctx, "Send me an email: carlos@ccextractor.org");

			nk_group_end(ctx);
		}

		nk_layout_row(ctx, NK_DYNAMIC, 27, 2, ok_ratio);
		nk_spacing(ctx, 1);
		if (nk_button_label(ctx, "OK"))
		{
			*show_about_ccx = nk_false;
			nk_popup_close(ctx);
		}
		nk_popup_end(ctx);
	}
	else
		*show_about_ccx = nk_false;
}

void draw_progress_details_popup(struct nk_context *ctx, int *show_progress_details, struct main_tab *main_settings)
{
	static struct nk_rect s = {20, 30, 480, 500};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Progress Details of Extraction", NK_WINDOW_CLOSABLE, s))
	{
		nk_layout_row_dynamic(ctx, 20, 1);
		for (int i = 0; i < main_settings->activity_string_count; i++)
			nk_label_wrap(ctx, main_settings->activity_string[i]);
		nk_popup_end(ctx);
	}
	else
		*show_progress_details = nk_false;
}

void draw_color_popup(struct nk_context *ctx, struct output_tab *output)
{
	static struct nk_rect s = {250, 250, 200, 230};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Color Picker", NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER, s))
	{
		nk_layout_row_dynamic(ctx, 160, 1);
		output->color_rgb = nk_color_picker(ctx, output->color_rgb, NK_RGBA);

		nk_layout_row_dynamic(ctx, 25, 3);
		nk_spacing(ctx, 1);
		if (nk_button_label(ctx, "OK"))
		{
			show_color_from_picker = nk_true;
			output->color_popup = nk_false;
			nk_popup_close(ctx);
		}
		nk_spacing(ctx, 1);

		nk_popup_end(ctx);
	}
	else
		output->color_popup = nk_false;
}

void draw_thread_popup(struct nk_context *ctx, int *show_thread_popup)
{
	static struct nk_rect s = {100, 100, 300, 175};
	static const float ratio[] = {0.85f, 0.15f};
	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "File Read Error",
			   NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER, s))
	{
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_label(ctx, "Cannot read file.", NK_TEXT_CENTERED);
		nk_layout_row_dynamic(ctx, 60, 1);
		nk_label_wrap(ctx, "Make sure the directory isn't write protected OR you are running the program with write permissions.");

		nk_layout_row(ctx, NK_DYNAMIC, 25, 2, ratio);
		nk_spacing(ctx, 1);
		if (nk_button_label(ctx, "OK"))
		{
			*show_thread_popup = nk_false;
			nk_popup_close(ctx);
		}
		nk_popup_end(ctx);
	}

	else
		*show_thread_popup = nk_false;
}
