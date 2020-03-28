//
// Created by Oleg Kisselef (olegkisselef at gmail dot com) on 6/21/15
//

#include <stdio.h>
#include <stdlib.h>
#include "ccx_share.h"
#include "ccx_common_option.h"
#include "ccx_decoders_structs.h"
#include "lib_ccx.h"

#ifdef ENABLE_SHARING

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

ccx_share_service_ctx ccx_share_ctx;

void ccx_sub_entry_msg_cleanup(CcxSubEntryMessage *msg)
{
	for (int i = 0; i < msg->n_lines; i++)
	{
		free(msg->lines[i]);
	}
	free(msg->lines);
	free(msg->stream_name);
}

void ccx_sub_entry_msg_print(CcxSubEntryMessage *msg)
{
	if (!msg)
	{
		dbg_print(CCX_DMT_SHARE, "[share] print(!msg)\n");
		return;
	}

	dbg_print(CCX_DMT_SHARE, "\n[share] sub msg #%lu\n", msg->counter);
	dbg_print(CCX_DMT_SHARE, "[share] name: %s\n", msg->stream_name);
	dbg_print(CCX_DMT_SHARE, "[share] start: %llu\n", msg->start_time);
	dbg_print(CCX_DMT_SHARE, "[share] end: %llu\n", msg->end_time);

	dbg_print(CCX_DMT_SHARE, "[share] lines count: %d\n", msg->n_lines);
	if (!msg->lines)
	{
		dbg_print(CCX_DMT_SHARE, "[share] no lines allocated\n");
		return;
	}

	dbg_print(CCX_DMT_SHARE, "[share] lines:\n");
	for (unsigned int i = 0; i < msg->n_lines; i++)
	{
		if (!msg->lines[i])
		{
			dbg_print(CCX_DMT_SHARE, "[share] line[%d] is not allocated\n", i);
		}
		dbg_print(CCX_DMT_SHARE, "[share] %s\n", msg->lines[i]);
	}
}

void ccx_sub_entries_init(ccx_sub_entries *entries)
{
	entries->count = 0;
	entries->messages = NULL;
}

void ccx_sub_entries_cleanup(ccx_sub_entries *entries)
{
	for (int i = 0; i < entries->count; i++)
	{
		ccx_sub_entry_msg_cleanup(entries->messages + i);
	}
	free(entries->messages);
	entries->messages = NULL;
	entries->count = 0;
}

void ccx_sub_entries_print(ccx_sub_entries *entries)
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_sub_entries_print (%u entries)\n", entries->count);
	for (int i = 0; i < entries->count; i++)
	{
		ccx_sub_entry_msg_print(entries->messages + i);
	}
}

ccx_share_status ccx_share_start(const char *stream_name) //TODO add stream
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: starting service\n");
	//TODO for multiple files we have to move creation to ccx_share_init
	ccx_share_ctx.nn_sock = nn_socket(AF_SP, NN_PUB);
	if (ccx_share_ctx.nn_sock < 0)
	{
		perror("[share] ccx_share_start: can't nn_socket()\n");
		fatal(EXIT_NOT_CLASSIFIED, "In ccx_share_start: can't nn_socket().");
	}

	if (!ccx_options.sharing_url)
	{
		ccx_options.sharing_url = strdup("tcp://*:3269");
	}

	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: url=%s\n", ccx_options.sharing_url);

	ccx_share_ctx.nn_binder = nn_bind(ccx_share_ctx.nn_sock, ccx_options.sharing_url);
	if (ccx_share_ctx.nn_binder < 0)
	{
		perror("[share] ccx_share_start: can't nn_bind()\n");
		fatal(EXIT_NOT_CLASSIFIED, "In ccx_share_start: can't nn_bind()");
	}

	int linger = -1;
	int rc = nn_setsockopt(ccx_share_ctx.nn_sock, NN_SOL_SOCKET, NN_LINGER, &linger, sizeof(int));
	if (rc < 0)
	{
		perror("[share] ccx_share_start: can't nn_setsockopt()\n");
		fatal(EXIT_NOT_CLASSIFIED, "In ccx_share_start: can't nn_setsockopt()");
	}

	//TODO remove path from stream name to minimize traffic (/?)
	ccx_share_ctx.stream_name = strdup(stream_name ? stream_name : "unknown");

	sleep(1); //We have to sleep a while, because it takes some time for subscribers to subscribe
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_stop()
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_stop: stopping service\n");
	nn_shutdown(ccx_share_ctx.nn_sock, ccx_share_ctx.nn_binder);
	free(ccx_share_ctx.stream_name);
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_send(struct cc_subtitle *sub)
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending\n");
	ccx_sub_entries entries;
	ccx_sub_entries_init(&entries);
	_ccx_share_sub_to_entries(sub, &entries);
	ccx_sub_entries_print(&entries);
	dbg_print(CCX_DMT_SHARE, "[share] entry obtained:\n");

	for (unsigned int i = 0; i < entries.count; i++)
	{
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: _sending %u\n", i);
		//TODO prevent sending empty messages
		if (_ccx_share_send(entries.messages + i) != CCX_SHARE_OK)
		{
			dbg_print(CCX_DMT_SHARE, "[share] can't send message\n");
			return CCX_SHARE_FAIL;
		}
	}

	ccx_sub_entries_cleanup(&entries);

	return CCX_SHARE_OK;
}

ccx_share_status _ccx_share_send(CcxSubEntryMessage *msg)
{
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send\n");
	size_t len = ccx_sub_entry_message__get_packed_size(msg);
	void *buf = malloc(len);
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: packing\n");
	ccx_sub_entry_message__pack(msg, buf);

	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: sending\n");
	int sent = nn_send(ccx_share_ctx.nn_sock, buf, len, 0);
	if (sent != len)
	{
		free(buf);
		dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: len=%zd sent=%d\n", len, sent);
		return CCX_SHARE_FAIL;
	}
	free(buf);
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: sent\n");
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_stream_done(char *stream_name)
{
	CcxSubEntryMessage msg = CCX_SUB_ENTRY_MESSAGE__INIT;
	msg.eos = 1;
	msg.stream_name = strdup(stream_name);
	msg.counter = 0;
	msg.start_time = 0;
	msg.end_time = 0;
	msg.n_lines = 0;
	msg.lines = NULL;

	if (_ccx_share_send(&msg) != CCX_SHARE_OK)
	{
		ccx_sub_entry_msg_cleanup(&msg);
		dbg_print(CCX_DMT_SHARE, "[share] can't send message\n");
		return CCX_SHARE_FAIL;
	}
	ccx_sub_entry_msg_cleanup(&msg);

	return CCX_SHARE_OK;
}

ccx_share_status _ccx_share_sub_to_entries(struct cc_subtitle *sub, ccx_sub_entries *entries)
{
	dbg_print(CCX_DMT_SHARE, "\n[share] _ccx_share_sub_to_entry\n");
	if (sub->type == CC_608)
	{
		dbg_print(CCX_DMT_SHARE, "[share] CC_608\n");
		struct eia608_screen *data;
		unsigned int nb_data = sub->nb_data;
		for (data = sub->data; nb_data; nb_data--, data++)
		{
			dbg_print(CCX_DMT_SHARE, "[share] data item\n");
			if (data->format == SFORMAT_XDS)
			{
				dbg_print(CCX_DMT_SHARE, "[share] XDS. Skipping\n");
				continue;
			}
			if (!data->start_time)
			{
				dbg_print(CCX_DMT_SHARE, "[share] No start time. Skipping\n");
				break;
			}

			entries->messages = realloc(entries->messages, ++entries->count * sizeof(CcxSubEntryMessage));
			if (!entries->messages)
			{
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In _ccx_share_sub_to_entry: Not enough memory to store sub-entry-messages\n");
			}

			unsigned int entry_index = entries->count - 1;
			CcxSubEntryMessage msg = CCX_SUB_ENTRY_MESSAGE__INIT;
			entries->messages[entry_index] = msg;

			entries->messages[entry_index].n_lines = 0;
			for (int i = 0; i < 15; i++)
			{
				if (data->row_used[i])
				{
					entries->messages[entry_index].n_lines++;
				}
			}
			if (!entries->messages[entry_index].n_lines)
			{ // Prevent writing empty screens. Not needed in .srt
				dbg_print(CCX_DMT_SHARE, "[share] buffer is empty\n");
				dbg_print(CCX_DMT_SHARE, "[share] done\n");
				return CCX_SHARE_OK;
			}
			entries->messages[entry_index].lines = (char **)malloc(entries->messages[entry_index].n_lines * sizeof(char *));
			if (!entries->messages[entry_index].lines)
			{
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In _ccx_share_sub_to_entry: Out of memory for entries->messages[entry_index].lines\n");
			}

			dbg_print(CCX_DMT_SHARE, "[share] Copying %u lines\n", entries->messages[entry_index].n_lines);
			int i = 0, j = 0;
			while (i < 15)
			{
				if (data->row_used[i])
				{
					entries->messages[entry_index].lines[j] =
					    (char *)malloc((strnlen((char *)data->characters[i], 32) + 1) * sizeof(char));
					if (!entries->messages[entry_index].lines[j])
					{
						fatal(EXIT_NOT_ENOUGH_MEMORY,
						      "In _ccx_share_sub_to_entry: Out of memory for entries->messages[entry_index].lines[j]\n");
					}
					strncpy(entries->messages[entry_index].lines[j], (char *)data->characters[i], 32);
					entries->messages[entry_index].lines[j][strnlen((char *)data->characters[i], 32)] = '\0';
					dbg_print(CCX_DMT_SHARE, "[share] line (len=%zd): %s\n",
						  strlen(entries->messages[entry_index].lines[j]),
						  entries->messages[entry_index].lines[j]);
					j++;
				}
				i++;
			}
			entries->messages[entry_index].eos = 0;
			entries->messages[entry_index].start_time = data->start_time;
			entries->messages[entry_index].end_time = data->end_time;
			entries->messages[entry_index].counter = ++ccx_share_ctx.counter;
			dbg_print(CCX_DMT_SHARE, "[share] item done\n");
		}
	}
	if (sub->type == CC_BITMAP)
	{
		dbg_print(CCX_DMT_SHARE, "[share] CC_BITMAP. Skipping\n");
	}
	if (sub->type == CC_RAW)
	{
		dbg_print(CCX_DMT_SHARE, "[share] CC_RAW. Skipping\n");
	}
	if (sub->type == CC_TEXT)
	{
		dbg_print(CCX_DMT_SHARE, "[share] CC_TEXT. Skipping\n");
	}
	dbg_print(CCX_DMT_SHARE, "[share] done\n");

	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_launch_translator(char *langs, char *auth)
{
	if (!langs)
	{
		fatal(EXIT_NOT_CLASSIFIED, "In ccx_share_launch_translator: Launching translator failed as target languages are not specified\n");
	}
	if (!auth)
	{
		fatal(EXIT_NOT_CLASSIFIED, "In ccx_share_launch_translator: Launching translator failed as no auth data is provided\n");
	}

	char buf[1024];
#ifdef _WIN32
	sprintf(buf, "start cctranslate -s=extractor -l=%s -k=%s", langs, auth);
#else
	sprintf(buf, "./cctranslate -s=extractor -l=%s -k=%s &", langs, auth);
#endif
	dbg_print(CCX_DMT_SHARE, "[share] launching translator: \"%s\"\n", buf);
	system(buf);
	return CCX_SHARE_OK;
}

#endif //ENABLE_SHARING
