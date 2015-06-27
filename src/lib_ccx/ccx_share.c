//
// Created by Oleg Kisselef (olegkisselef at gmail dot com) on 6/21/15
//
#ifdef ENABLE_SHARING

#include <stdio.h>
#include <stdlib.h>
#include "ccx_share.h"
#include "ccx_common_option.h"
#include "ccx_decoders_structs.h"
#include "lib_ccx.h"
#include "zmq.h"

void ccx_sub_entry_init(ccx_sub_entry *entry)
{
	entry->counter = 0;
	entry->start_time = 0;
	entry->end_time = 0;
	entry->lines = NULL;
	entry->lines_count = 0;
}

void ccx_sub_entry_cleanup(ccx_sub_entry *entry)
{
	for (int i = 0; i < entry->lines_count; i++) {
		free(entry->lines[i]);
	}
	free(entry->lines);
}

void ccx_sub_entry_print(ccx_sub_entry *entry)
{
	if (!entry) {
		dbg_print(CCX_DMT_SHARE, "[share] !entry\n");
		return;
	}

	dbg_print(CCX_DMT_SHARE, "\n[share] sub entry #%lu\n", entry->counter);
	dbg_print(CCX_DMT_SHARE, "[share] start: %llu\n", entry->start_time);
	dbg_print(CCX_DMT_SHARE, "[share] end: %llu\n", entry->end_time);

	dbg_print(CCX_DMT_SHARE, "[share] lines count: %d\n", entry->lines_count);
	if (!entry->lines) {
		dbg_print(CCX_DMT_SHARE, "[share] no lines allocated\n");
		return;
	}

	dbg_print(CCX_DMT_SHARE, "[share] lines:\n");
	for (unsigned int i = 0; i < entry->lines_count; i++) {
		if (!entry->lines[i]) {
			dbg_print(CCX_DMT_SHARE, "[share] line[%d] is not allocated\n", i);
		}
		dbg_print(CCX_DMT_SHARE, "[share] %s\n", entry->lines[i]);
	}
}

void ccx_sub_entries_init(ccx_sub_entries *entries)
{
	entries->count = 0;
	entries->entries = NULL;
}

void ccx_sub_entries_cleanup(ccx_sub_entries *entries)
{
	for (int i = 0; i < entries->count; i++) {
		ccx_sub_entry_cleanup(&(entries->entries[i]));
	}
	free(entries->entries);
	entries->entries = NULL;
	entries->count = 0;
}

void ccx_sub_entries_print(ccx_sub_entries *entries)
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_sub_entries_print (%u entries)\n", entries->count);
	for (int i = 0; i < entries->count; i++) {
	ccx_sub_entry_print(&(entries->entries[i]));
	}
}


ccx_share_status ccx_share_start(const char *stream_name) //TODO add stream
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: starting service\n");
	//TODO for multiple files we have to move creation to ccx_share_init
	ccx_share.zmq_ctx = zmq_ctx_new();
	ccx_share.zmq_sock = zmq_socket(ccx_share.zmq_ctx, ZMQ_PUB);

	if (!ccx_options.sharing_url) {
		ccx_options.sharing_url = strdup("tcp://*:3269");
	}

	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: url=%s\n", ccx_options.sharing_url);

	int rc = zmq_bind(ccx_share.zmq_sock, ccx_options.sharing_url);
	if (rc) {
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: cant zmq_bind()\n");
		fatal(EXIT_NOT_CLASSIFIED, "ccx_share_start");
	}
	//TODO remove path from stream name to minimize traffic (/?)
	ccx_share.stream_name = strdup(stream_name ? stream_name : "unknown");
	sleep(1); //We have to sleep a while, because it takes some time for subscribers to subscribe
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_stop()
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_stop: stopping service\n");
	const size_t buf_size = 128;
	char buf[buf_size];
	int rc = zmq_getsockopt (ccx_share.zmq_sock, ZMQ_LAST_ENDPOINT, buf, (size_t *)&buf_size);
	assert (rc == 0);
	/* Unbind socket by real endpoint */
	rc = zmq_unbind (ccx_share.zmq_sock, buf);
	assert (rc == 0);
	zmq_close(ccx_share.zmq_sock);
	zmq_ctx_destroy(ccx_share.zmq_ctx);
	free(ccx_share.stream_name);
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_send(struct cc_subtitle *sub)
{
	dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending\n");
	ccx_sub_entries entries;
	ccx_sub_entries_init(&entries);
	_ccx_share_sub_to_entry(sub, &entries);
	ccx_sub_entries_print(&entries);
	dbg_print(CCX_DMT_SHARE, "[share] entry obtained:\n");

	ccx_sub_entry *entry;
	for (unsigned int i = 0; i < entries.count; i++) {
		entry = &entries.entries[i];
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending %u\n", i);
		PbSubEntry msg = PB_SUB_ENTRY__INIT;
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending %u: inited\n", i);
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending name %s\n", ccx_share.stream_name);
		msg.stream_name = strdup(ccx_share.stream_name);
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending %u: duped\n", i);
		msg.has_counter = 1;
		msg.counter = (int64_t) entry->counter;
		msg.has_start_time = 1;
		msg.start_time = (int64_t) entry->start_time;
		msg.has_end_time = 1;
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending %u: middle\n", i);
		msg.end_time = (int64_t) entry->end_time;
		msg.has_lines_count = 1;
		msg.lines_count = entry->lines_count;
		msg.n_lines = entry->lines_count;
		msg.lines = (char **) malloc(msg.lines_count * sizeof(char *));
		if (!msg.lines) {
			fatal(EXIT_NOT_ENOUGH_MEMORY, "[share] msg\n");
		}
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: sending %u: allocated\n", i);
		for (int j = 0; j < msg.lines_count; j++) {
			msg.lines[j] = strdup(entry->lines[j]);
			if (!msg.lines[j]) {
				fatal(EXIT_NOT_ENOUGH_MEMORY, "[share] msg[j]\n");
			}
		}
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: _sending %u\n", i);
		if (_ccx_share_send(&msg) != CCX_SHARE_OK) {
			dbg_print(CCX_DMT_SHARE, "[share] can't send message\n");
			return CCX_SHARE_FAIL;
		}

		for (int j = 0; j < msg.lines_count; j++) {
			free(msg.lines[j]);
		}
		free(msg.lines);
	}

	ccx_sub_entries_cleanup(&entries);
	//sleep(1);
	return CCX_SHARE_OK;
}

ccx_share_status _ccx_share_send(PbSubEntry *msg)
{
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send\n");
	size_t len = pb_sub_entry__get_packed_size(msg);
	void *buf = malloc(len);
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: packing\n");
	pb_sub_entry__pack(msg, buf);

	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: sending\n");
	int sent = zmq_send(ccx_share.zmq_sock, buf, len, 0);
	if (sent != len) {
		dbg_print(CCX_DMT_SHARE, "[share] ccx_share_send: len=%zd sent=%d\n", len, sent);
		return CCX_SHARE_FAIL;
	}
	free(buf);
	dbg_print(CCX_DMT_SHARE, "[share] _ccx_share_send: sent\n");
	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_stream_done()
{
	PbSubEntry msg = PB_SUB_ENTRY__INIT;
	msg.eos = 1;
	msg.stream_name = strdup(ccx_share.stream_name);
	msg.has_lines_count = 0;
	msg.has_counter = 0;
	msg.has_start_time = 0;
	msg.has_end_time = 0;
	msg.n_lines = 0;
	msg.lines = NULL;

	if (_ccx_share_send(&msg) != CCX_SHARE_OK) {
		dbg_print(CCX_DMT_SHARE, "[share] can't send message\n");
		return CCX_SHARE_FAIL;
	}

	return CCX_SHARE_OK;
}

ccx_share_status _ccx_share_sub_to_entry(struct cc_subtitle *sub, ccx_sub_entries *ents)
{
	dbg_print(CCX_DMT_SHARE, "\n[share] _ccx_share_sub_to_entry\n");
	if (sub->type == CC_608) {
		dbg_print(CCX_DMT_SHARE, "[share] CC_608\n");
		struct eia608_screen *data;
		unsigned int nb_data = sub->nb_data;
		for (data = sub->data; nb_data; nb_data--, data++) {
			dbg_print(CCX_DMT_SHARE, "[share] data item\n");
			if (data->format == SFORMAT_XDS) {
				dbg_print(CCX_DMT_SHARE, "[share] XDS. Skipping\n");
				continue;
			}
			if (!data->start_time) {
				dbg_print(CCX_DMT_SHARE, "[share] No start time. Skipping\n");
				break;
			}
			ents->entries = realloc(ents->entries, ++ents->count * sizeof(ccx_sub_entry));
			if (!ents->entries) {
				fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry\n");
			}
			unsigned int entry_index = ents->count - 1;
			ents->entries[entry_index].lines_count = 0;
			for (int i = 0; i < 15; i++) {
				if (data->row_used[i]) {
					ents->entries[entry_index].lines_count++;
				}
			}
			if (!ents->entries[entry_index].lines_count) {// Prevent writing empty screens. Not needed in .srt
				dbg_print(CCX_DMT_SHARE, "[share] buffer is empty\n");
				dbg_print(CCX_DMT_SHARE, "[share] done\n");
				return CCX_SHARE_OK;
			}
			ents->entries[entry_index].lines = (char **) malloc(ents->entries[entry_index].lines_count * sizeof(char *));
			if (!ents->entries[entry_index].lines) {
				fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry: ents->entries[entry_index].lines\n");
			}

			dbg_print(CCX_DMT_SHARE, "[share] Copying %u lines\n", ents->entries[entry_index].lines_count);
			int i = 0, j = 0;
			while (i < 15) {
				if (data->row_used[i]) {
					ents->entries[entry_index].lines[j] =
						(char *) malloc((strnlen((char *) data->characters[i], 32) + 1) * sizeof(char));
					if (!ents->entries[entry_index].lines[j]) {
						fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry: ents->entries[entry_index].lines[j]\n");
					}
					strncpy(ents->entries[entry_index].lines[j], (char *) data->characters[i], 32);
					ents->entries[entry_index].lines[j][strnlen((char *) data->characters[i], 32)] = '\0';
					dbg_print(CCX_DMT_SHARE, "[share] line (len=%zd): %s\n",
								strlen(ents->entries[entry_index].lines[j]),
								ents->entries[entry_index].lines[j]);
					j++;
				}
				i++;
			}
			ents->entries[entry_index].start_time = (unsigned long long) data->start_time;
			ents->entries[entry_index].end_time = (unsigned long long) data->end_time;
			ents->entries[entry_index].counter = ++ccx_share.counter;
			dbg_print(CCX_DMT_SHARE, "[share] item done\n");
		}
	}
	if (sub->type == CC_BITMAP) {
		dbg_print(CCX_DMT_SHARE, "[share] CC_BITMAP. Skipping\n");
	}
	if (sub->type == CC_RAW) {
		dbg_print(CCX_DMT_SHARE, "[share] CC_RAW. Skipping\n");
	}
	if (sub->type == CC_TEXT) {
		dbg_print(CCX_DMT_SHARE, "[share] CC_TEXT. Skipping\n");
	}
	dbg_print(CCX_DMT_SHARE, "[share] done\n");

	return CCX_SHARE_OK;
}

ccx_share_status ccx_share_launch_translator(char *langs, char *auth)
{
	if (!langs) {
		fatal(EXIT_NOT_CLASSIFIED, "[translate] launching translator failed: target languages not specified\n");
	}
	if (!auth) {
		fatal(EXIT_NOT_CLASSIFIED, "[translate] launching translator failed: no auth data provided\n");
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
