#include "ccx_demuxer.h"
#include "ccx_common_common.h"

int need_capInfo(struct ccx_demuxer *ctx)
{
	struct cap_info* iter; 
	if(list_empty(&ctx->cinfo_tree.all_stream))
	{
		return CCX_TRUE;
	}

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->codec == CCX_CODEC_NONE)
			return CCX_TRUE;
		if (iter->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return CCX_TRUE;
	}

	return CCX_FALSE;
}

int count_complete_capInfo(struct ccx_demuxer *ctx)
{
	int count = 0;
	struct cap_info* iter; 

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->codec == CCX_CODEC_NONE)
			return CCX_TRUE;
		if (iter->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return CCX_TRUE;
		count++;
	}
	return count;
}

void ignore_other_stream(struct ccx_demuxer *ctx, int pid)
{
	struct cap_info* iter; 
	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->pid == pid)
			iter->ignore = 1;
	}
}
int get_best_stream(struct ccx_demuxer *ctx)
{
	struct cap_info* iter;

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->codec == CCX_CODEC_TELETEXT)
			return iter->pid;
	}
	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->codec == CCX_CODEC_DVB)
			return iter->pid;
	}

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
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
		if(ptr->stream_pid == pid)
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

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if (iter->pid == pid && iter->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM)
			return CCX_TRUE;
	}
	return CCX_FALSE;
}

int update_capinfo(struct ccx_demuxer *ctx, int pid, enum ccx_stream_type stream, enum ccx_code_type codec)
{
	struct cap_info* ptr;
	struct cap_info* tmp;
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

	list_for_each_entry(tmp ,&ptr->all_stream, all_stream)
	{
		if (tmp->pid == pid)
		{
			if(stream != CCX_STREAM_TYPE_UNKNOWNSTREAM)
				tmp->stream = stream;
			if(codec != CCX_CODEC_NONE)
				tmp->codec = codec;

			tmp->saw_pesstart = 0;
			tmp->capbuflen = 0;
			tmp->capbufsize = 0;
			tmp->ignore = 0;
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

	tmp->saw_pesstart = 0;
	tmp->capbuflen = 0;
	tmp->capbufsize = 0;
	tmp->capbuf = NULL;
	tmp->ignore = 0;

	list_add_tail( &(tmp->all_stream), &(ptr->all_stream) );

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

	list_for_each_entry(iter ,&ctx->cinfo_tree.all_stream, all_stream)
	{
		if(iter->pid == pid && iter->codec != CCX_CODEC_NONE)
			return iter;
	}
	return NULL;
}

