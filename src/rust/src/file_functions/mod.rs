/**
 * Read from buffer if there is insufficient data then cache the buffer
 *
 * @param ctx ccx_demuxer context properly initialized ccx_demuxer with some input
 *            Not to be NULL, since ctx is deferenced inside this function
 *
 * @param buffer if buffer then it must be allocated to at least bytes len as
 *               passed in third argument, If buffer is NULL then those number of bytes
 *               are skipped from input.
 * @param bytes number of bytes to be read from file buffer.
 *
 * @return 0 or number of bytes, if returned 0 then op should check error number to know
 *              details of error
 */
pub mod file;
