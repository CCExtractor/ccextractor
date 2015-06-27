//
// Created by Oleg Kisselef (olegkisselef at gmail dot com) on 6/21/15
//

#ifdef ENABLE_SHARING

#ifndef CCEXTRACTOR_CCX_SHARE_H
#define CCEXTRACTOR_CCX_SHARE_H

#include "ccx_common_platform.h"
#include "ccx_common_structs.h"
#include "ccx_sub_entry.pb_c.h"

typedef struct ccx_sub_entry {
	unsigned long counter;
	unsigned long long start_time;
	unsigned long long end_time;
	unsigned int lines_count;
	char **lines;
} ccx_sub_entry;

typedef struct ccx_sub_entries {
	ccx_sub_entry *entries;
	unsigned int count;
} ccx_sub_entries;

typedef struct ccx_share_service {
	long port;
	unsigned long counter;
	char *stream_name;
	void *zmq_ctx;
	void *zmq_sock;
} ccx_share_service;

ccx_share_service ccx_share;

typedef enum ccx_share_status {
	CCX_SHARE_OK = 0,
	CCX_SHARE_FAIL
} ccx_share_status;

void ccx_sub_entry_init(ccx_sub_entry *);
void ccx_sub_entry_cleanup(ccx_sub_entry *);
void ccx_sub_entry_print(ccx_sub_entry *);

void ccx_sub_entries_init(ccx_sub_entries *);
void ccx_sub_entries_cleanup(ccx_sub_entries *);
void ccx_sub_entries_print(ccx_sub_entries *);

ccx_share_status ccx_share_launch_translator(char *langs, char *google_api_key);
ccx_share_status ccx_share_start(const char *);
ccx_share_status ccx_share_stop();
ccx_share_status ccx_share_send(struct cc_subtitle *);
ccx_share_status ccx_share_stream_done();
ccx_share_status _ccx_share_sub_to_entry(struct cc_subtitle *, ccx_sub_entries *);
ccx_share_status _ccx_share_send(PbSubEntry *);

#endif //CCEXTRACTOR_CCX_SHARE_H

#endif //ENABLE_SHARING