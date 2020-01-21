#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

/**
 * Read from buffer if there is insufficient data then cache the buffer
 *
 * @param ctx ccx_demuxer context properly initilaized ccx_demuxer with some input
 *            Not to be NULL, since ctx is derefrenced inside this function
 *
 * @param buffer if buffer then it must be allocated to at;east bytes len as 
 *               passed in third argument, If buffer is NULL then those number of bytes
 *               are skipped from input.
 * @param bytes number of bytes to be read from file buffer.
 *
 * @return 0 or number of bytes, if returned 0 then op should check error number to know 
 *              details of error 
 */
size_t buffered_read_opt (struct ccx_demuxer *ctx, unsigned char *buffer, size_t bytes);


/**
 * Skip bytes from file buffer and if needed also seek file for number of bytes.
 *
 */
static size_t inline  buffered_skip(struct ccx_demuxer *ctx, unsigned int bytes)
{
	size_t result;
	if (bytes <= ctx->bytesinbuffer - ctx->filebuffer_pos)
	{
		ctx->filebuffer_pos += bytes;
		result = bytes;
	}
	else
	{
		result = buffered_read_opt (ctx, NULL, bytes);
	}
	return result;
}

/**
 * Read bytes from file buffer and if needed also read file for number of bytes.
 *
 */
static size_t inline buffered_read(struct ccx_demuxer *ctx, unsigned char *buffer, size_t bytes)
{
	size_t result;
	if (bytes <= ctx->bytesinbuffer - ctx->filebuffer_pos)
	{
		if (buffer != NULL)
			memcpy (buffer, ctx->filebuffer + ctx->filebuffer_pos, bytes);
		ctx->filebuffer_pos+=bytes;
		result = bytes;
	}
	else
	{
		result = buffered_read_opt (ctx, buffer, bytes);
		if (ccx_options.gui_mode_reports && ccx_options.input_source == CCX_DS_NETWORK)
		{
			net_activity_gui++;
			if (!(net_activity_gui%1000))
				activity_report_data_read();
		}
	}
	return result;
}

/**
 * Read single byte from file buffer and if needed also read file for number of bytes.
 *
 */
static size_t inline buffered_read_byte(struct ccx_demuxer *ctx, unsigned char *buffer)
{
	size_t result = 0;
	if (ctx->bytesinbuffer - ctx->filebuffer_pos)
	{
		if (buffer)
		{
			*buffer=ctx->filebuffer[ctx->filebuffer_pos];
			ctx->filebuffer_pos++;
			result = 1;
		}
	}
	else
		result = buffered_read_opt (ctx, buffer, 1);

	return result;
}

unsigned short buffered_get_be16(struct ccx_demuxer *ctx);
unsigned char buffered_get_byte (struct ccx_demuxer *ctx);
unsigned int buffered_get_be32(struct ccx_demuxer *ctx);
unsigned short buffered_get_le16(struct ccx_demuxer *ctx);
unsigned int buffered_get_le32(struct ccx_demuxer *ctx);
uint64_t buffered_get_be64(struct ccx_demuxer *ctx);
#endif
