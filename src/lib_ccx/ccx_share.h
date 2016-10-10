//
// Created by Oleg Kisselef (olegkisselef at gmail dot com) on 6/21/15
//

#ifndef CCEXTRACTOR_CCX_SHARE_H
#define CCEXTRACTOR_CCX_SHARE_H

#include "ccx_common_platform.h"
#include "ccx_common_structs.h"
#include "ccx_sub_entry_message.pb-c.h"
#include "lib_ccx.h"

#ifdef ENABLE_SHARING

typedef struct _ccx_sub_entries {
	CcxSubEntryMessage *messages;
	unsigned int count;
} ccx_sub_entries;

typedef struct _ccx_share_service_ctx {
	LLONG counter;
	char *stream_name;
	int nn_sock;
	int nn_binder;
} ccx_share_service_ctx;

extern ccx_share_service_ctx ccx_share_ctx;

typedef enum _ccx_share_status {
	CCX_SHARE_OK = 0,
	CCX_SHARE_FAIL
} ccx_share_status;

void ccx_sub_entry_message_cleanup(CcxSubEntryMessage *);
void ccx_sub_entry_message_print(CcxSubEntryMessage *);

void ccx_sub_entries_init(ccx_sub_entries *);
void ccx_sub_entries_cleanup(ccx_sub_entries *);
void ccx_sub_entries_print(ccx_sub_entries *);

ccx_share_status ccx_share_launch_translator(char *langs, char *google_api_key);
ccx_share_status ccx_share_start(const char *);
ccx_share_status ccx_share_stop();
ccx_share_status ccx_share_send(struct cc_subtitle *);
ccx_share_status ccx_share_stream_done(char *);
ccx_share_status _ccx_share_sub_to_entries(struct cc_subtitle *, ccx_sub_entries *);
ccx_share_status _ccx_share_send(CcxSubEntryMessage *);

#endif //ENABLE_SHARING

#endif //CCEXTRACTOR_CCX_SHARE_H
