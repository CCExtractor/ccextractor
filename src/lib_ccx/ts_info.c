#include "ccx_demuxer.h"
#include "ccx_common_common.h"
#include "lib_ccx.h"
#include "dvb_subtitle_decoder.h" 

/**
    We need stream info from PMT table when any of the following Condition meets:
    1) Dont have any caption stream registered to be extracted
    2) Want a streams per program, and program_number has never been registered
    3) We need single stream and its info about codec and buffertype is incomplete
*/
int need_capInfo(struct ccx_demuxer *ctx, int program_number)
{
	struct cap_info* iter; 
	if(list_empty(&ctx->cinfo_tree.all_stream))
	{
		return CCX_TRUE;
	}

	if(ctx->ts_allprogram == CCX_TRUE)
	{
		list_for_each_entry(iter, &ctx->cinfo_tree.pg_stream, pg_stream, struct cap_info)
		{
			if (iter->program_number == program_number)
				return CCX_FALSE;
		}
		return CCX_TRUE;
	}
	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->codec == CCX_CODEC_NONE)
			return CCX_TRUE;
		if (iter->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return CCX_TRUE;
	}

	return CCX_FALSE;
}

void ignore_other_stream(struct ccx_demuxer *ctx, int pid)
{
	struct cap_info* iter; 
	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->pid != pid)
			iter->ignore = 1;
	}
}

int get_programme_number(struct ccx_demuxer *ctx, int pid)
{
	struct cap_info* iter;
	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->pid == pid)
			return iter->program_number;
	}
	return CCX_UNKNOWN;
}

struct cap_info *get_sib_stream_by_type(struct cap_info* program, enum ccx_code_type type)
{
	struct cap_info* iter;
	list_for_each_entry(iter, &program->sib_head, sib_stream, struct cap_info)
	{
		if(iter->codec == type)
			return iter;
	}
	return NULL;
}
struct cap_info* get_best_sib_stream(struct cap_info* program)
{
	struct cap_info* info;

	info = get_sib_stream_by_type(program, CCX_CODEC_TELETEXT);
	if(info)
		return info;

	info = get_sib_stream_by_type(program, CCX_CODEC_DVB);
	if(info)
		return info;

	info = get_sib_stream_by_type(program, CCX_CODEC_ATSC_CC);
	if(info)
		return info;

	return NULL;
}

void ignore_other_sib_stream(struct cap_info* head, int pid)
{
	struct cap_info* iter; 
	list_for_each_entry(iter, &head->sib_head, sib_stream, struct cap_info)
	{
		if(iter->pid != pid)
			iter->ignore = 1;
	}
}

int get_best_stream(struct ccx_demuxer *ctx)
{
	struct cap_info* iter;

	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->codec == CCX_CODEC_TELETEXT)
			return iter->pid;
	}
	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->codec == CCX_CODEC_DVB)
			return iter->pid;
	}

	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->codec == CCX_CODEC_ATSC_CC)
			return iter->pid;
	}

	return -1;
}

struct demuxer_data *get_data_stream(struct demuxer_data *data, int pid)
{
	struct demuxer_data *ptr = data;
	for(ptr = data; ptr; ptr = ptr->next_stream)
		if(ptr->stream_pid == pid && ptr->len > 0)
			return ptr;

	return NULL;
}
int need_capInfo_for_pid(struct ccx_demuxer *ctx, int pid)
{
	struct cap_info* iter; 
	if(list_empty(&ctx->cinfo_tree.all_stream))
	{
		return CCX_FALSE;
	}

	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if (iter->pid == pid && iter->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return CCX_TRUE;
	}
	return CCX_FALSE;
}

int update_capinfo(struct ccx_demuxer *ctx, int pid, enum ccx_stream_type stream, enum ccx_code_type codec, int pn, void *private_data)
{
	struct cap_info* ptr;
	struct cap_info* tmp;
	struct cap_info* program_iter;
	if(!ctx)
	{
		errno = EINVAL;
		return -1;
	}

	if (ctx->ts_datastreamtype != CCX_STREAM_TYPE_UNKNOWNSTREAM && ctx->ts_datastreamtype != stream)
	{
		errno = EINVAL;
		return -1;
	}

	ptr = &ctx->cinfo_tree;

	list_for_each_entry(tmp, &ptr->all_stream, all_stream, struct cap_info)
	{
		if (tmp->pid == pid)
		{
			if(stream != CCX_STREAM_TYPE_UNKNOWNSTREAM || codec != CCX_CODEC_NONE)
			{
				tmp->stream = stream;
				tmp->codec = codec;

				tmp->saw_pesstart = 0;
				tmp->capbuflen = 0;
				tmp->capbufsize = 0;
				tmp->ignore = 0;
				tmp->codec_private_data = private_data;
			}
			return CCX_OK;
		}
	}
	tmp = malloc(sizeof(struct cap_info));
	if(!tmp)
	{
		return -1;
	}

	tmp->pid = pid;
	tmp->stream = stream;
	tmp->codec = codec;
	tmp->program_number = pn;

	tmp->saw_pesstart = 0;
	tmp->capbuflen = 0;
	tmp->capbufsize = 0;
	tmp->capbuf = NULL;
	tmp->ignore = CCX_FALSE;
	if(!private_data)
	{
		switch(tmp->codec)
		{
		case CCX_CODEC_TELETEXT:
			tmp->codec_private_data = telxcc_init();
			break;
		case CCX_CODEC_DVB:
			tmp->codec_private_data = dvbsub_init_decoder(NULL);
			break;
		default:
			tmp->codec_private_data = NULL;
		}
	}
	else
		tmp->codec_private_data = private_data;

	list_add_tail( &(tmp->all_stream), &(ptr->all_stream) );

	list_for_each_entry(program_iter, &ptr->pg_stream, pg_stream, struct cap_info)
	{
		if (program_iter->program_number == pn)
		{
			list_add_tail( &(tmp->sib_stream), &(program_iter->sib_head) );
			return CCX_OK;
		}
	}
	INIT_LIST_HEAD(&(tmp->sib_head));
	list_add_tail( &(tmp->sib_stream), &(tmp->sib_head) );
	list_add_tail( &(tmp->pg_stream), &(ptr->pg_stream) );

	return 0;
}

void dinit_cap (struct ccx_demuxer *ctx)
{
	struct cap_info* iter; 
	while(!list_empty(&ctx->cinfo_tree.all_stream))
	{
		iter = list_entry(ctx->cinfo_tree.all_stream.next, struct cap_info, all_stream);
		list_del(&iter->all_stream);
		free(iter);
	}
}


struct cap_info * get_cinfo(struct ccx_demuxer *ctx, int pid)
{
	struct cap_info* iter; 

	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		if(iter->pid == pid && iter->codec != CCX_CODEC_NONE && iter->stream != CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return iter;
	}
	return NULL;
}

