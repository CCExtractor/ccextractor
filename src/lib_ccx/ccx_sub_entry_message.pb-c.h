/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: ccx_sub_entry_message.proto */

#ifndef PROTOBUF_C_ccx_5fsub_5fentry_5fmessage_2eproto__INCLUDED
#define PROTOBUF_C_ccx_5fsub_5fentry_5fmessage_2eproto__INCLUDED

#include "protobuf-c.h"
#include "lib_ccx.h"

#ifdef ENABLE_SHARING

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1001001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _CcxSubEntryMessage CcxSubEntryMessage;


/* --- enums --- */


/* --- messages --- */

struct  _CcxSubEntryMessage
{
	ProtobufCMessage base;
	int32_t eos;
	char *stream_name;
	int64_t counter;
	int64_t start_time;
	int64_t end_time;
	size_t n_lines;
	char **lines;
};
#define CCX_SUB_ENTRY_MESSAGE__INIT \
{
	PROTOBUF_C_MESSAGE_INIT (&ccx_sub_entry_message__descriptor) \
    , 0, NULL, 0, 0, 0, 0,NULL
}


/* CcxSubEntryMessage methods */
void   ccx_sub_entry_message__init
(CcxSubEntryMessage         *message);
size_t ccx_sub_entry_message__get_packed_size
(const CcxSubEntryMessage   *message);
size_t ccx_sub_entry_message__pack
(const CcxSubEntryMessage   *message,
uint8_t             *out);
size_t ccx_sub_entry_message__pack_to_buffer
(const CcxSubEntryMessage   *message,
ProtobufCBuffer     *buffer);
CcxSubEntryMessage *
ccx_sub_entry_message__unpack
(ProtobufCAllocator  *allocator,
size_t               len,
const uint8_t       *data);
void   ccx_sub_entry_message__free_unpacked
(CcxSubEntryMessage *message,
ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void(*CcxSubEntryMessage_Closure)
(const CcxSubEntryMessage *message,
void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor ccx_sub_entry_message__descriptor;

PROTOBUF_C__END_DECLS

#endif //ENABLE_SHARING

#endif  /* PROTOBUF_C_ccx_5fsub_5fentry_5fmessage_2eproto__INCLUDED */
