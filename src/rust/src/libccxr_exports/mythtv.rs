use crate::bindings::ccx_demuxer;
use crate::bindings::AVPacketMythTV;
use crate::ccx_options;
use crate::common::copy_to_rust;
use crate::demuxer::common_types::CcxDemuxer;
use crate::ffi_alloc;
use crate::libccxr_exports::demuxer::{
    copy_demuxer_from_c_to_rust, copy_read_position_from_c, copy_read_position_to_c,
};
use crate::mythtv::common_structs::AVPacketMythTv;
use crate::mythtv::myth::mpegps_read_packet;
use lib_ccxr::common::Options;
use lib_ccxr::fatal;
use lib_ccxr::util::log::ExitCause;
use std::os::raw::{c_int, c_uchar, c_uint, c_void};
use std::ptr;

/// Rust state kept for one `myth_loop()` run.
///
/// The demuxer and the options are copied from C once, when the loop starts.
/// For each packet only the read position is synced, because a full demuxer
/// copy rebuilds the PID tables and costs far more than parsing the packet.
pub struct MythReader {
    demux: CcxDemuxer<'static>,
    options: Options,
    packet: AVPacketMythTv,
}

/// Creates the reader used by [`ccxr_mpegps_read_packet`].
///
/// Returns NULL if `ctx` is NULL. Free the reader with [`ccxr_myth_reader_free`].
///
/// # Safety
///
/// `ctx` must be NULL or point to a valid `ccx_demuxer`, and `ccx_options`
/// must be initialized.
#[no_mangle]
pub unsafe extern "C" fn ccxr_myth_reader_new(ctx: *mut ccx_demuxer) -> *mut MythReader {
    if ctx.is_null() {
        return ptr::null_mut();
    }
    let reader = MythReader {
        demux: copy_demuxer_from_c_to_rust(ctx),
        options: copy_to_rust(&raw const ccx_options),
        packet: AVPacketMythTv::default(),
    };
    Box::into_raw(Box::new(reader))
}

/// Frees a reader created by [`ccxr_myth_reader_new`]. Does nothing for NULL.
///
/// # Safety
///
/// `reader` must be NULL or a pointer returned by [`ccxr_myth_reader_new`]
/// that has not been freed yet.
#[no_mangle]
pub unsafe extern "C" fn ccxr_myth_reader_free(reader: *mut MythReader) {
    if !reader.is_null() {
        drop(Box::from_raw(reader));
    }
}

/// Copies a packet into the C `AVPacketMythTV`.
///
/// `av_c.data` is owned by myth.c, which grows it with `realloc()` and frees
/// it with `free()`, so it is resized with C's allocator as well.
///
/// # Safety
///
/// `av_c` must point to a valid `AVPacketMythTV` whose `data` is NULL or was
/// allocated by C's allocator.
pub unsafe fn copy_av_packet_from_rust_to_c(av_rust: &AVPacketMythTv, av_c: *mut AVPacketMythTV) {
    let av_packet = &mut *av_c;

    av_packet.pts = av_rust.pts;
    av_packet.dts = av_rust.dts;
    av_packet.stream_index = av_rust.stream_index;
    av_packet.flags = av_rust.flags;
    av_packet.duration = av_rust.duration;
    av_packet.priv_ = av_rust.priv_data;
    av_packet.pos = av_rust.pos;
    av_packet.codec_id = av_rust.codec_id.to_c();
    av_packet.type_ = av_rust.ty.to_c();

    let size = av_rust.data.len();
    // realloc(ptr, 0) may free the buffer and return NULL, so always ask for
    // at least one byte.
    let data = ffi_alloc::realloc(av_packet.data as *mut c_void, size.max(1)) as *mut c_uchar;
    if data.is_null() {
        fatal!(
            cause = ExitCause::NotEnoughMemory;
            "Not enough memory, realloc() failed while reading mpeg packets. Giving up.\n"
        );
    }
    ptr::copy_nonoverlapping(av_rust.data.as_ptr(), data, size);
    av_packet.data = data;
    av_packet.size = size as c_int;
}

/// Rust version of `mpegps_read_packet()` in myth.c.
///
/// Reads the next packet into `av` and returns 0, or returns a negative value
/// at the end of the input.
///
/// # Safety
///
/// `reader` must come from [`ccxr_myth_reader_new`] for the same `ctx`.
/// `header_state` must point to a `c_uint`, `psm_es_type` to 256 bytes, and
/// `av` to a valid `AVPacketMythTV` as described in
/// [`copy_av_packet_from_rust_to_c`].
#[no_mangle]
pub unsafe extern "C" fn ccxr_mpegps_read_packet(
    reader: *mut MythReader,
    ctx: *mut ccx_demuxer,
    header_state: *mut c_uint,
    psm_es_type: *mut c_uchar,
    av: *mut AVPacketMythTV,
) -> c_int {
    if reader.is_null()
        || ctx.is_null()
        || header_state.is_null()
        || psm_es_type.is_null()
        || av.is_null()
    {
        return -1;
    }
    let reader = &mut *reader;
    let psm_es_type = &mut *(psm_es_type as *mut [c_uchar; 256]);

    copy_read_position_from_c(ctx, &mut reader.demux);
    let result = mpegps_read_packet(
        &mut reader.demux,
        &mut *header_state,
        psm_es_type,
        &mut reader.options,
        &mut reader.packet,
    );
    copy_read_position_to_c(ctx, &reader.demux);
    // Reading stdin to the end turns live_stream off; the C side has to see that.
    ccx_options.live_stream = reader
        .options
        .live_stream
        .map_or(-1, |live_stream| live_stream.seconds() as _);

    if result == 0 {
        copy_av_packet_from_rust_to_c(&reader.packet, av);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::mythtv::common_structs::{CodecIDMythTv, CodecTypeMythTv};

    fn empty_c_packet() -> AVPacketMythTV {
        AVPacketMythTV {
            pts: 0,
            dts: 0,
            data: ptr::null_mut(),
            size: 0,
            stream_index: 0,
            flags: 0,
            duration: 0,
            destruct: None,
            priv_: ptr::null_mut(),
            pos: 0,
            codec_id: 0,
            type_: 0,
        }
    }

    fn rust_packet(data: Vec<u8>) -> AVPacketMythTv {
        AVPacketMythTv {
            pts: 123456789,
            dts: 987654321,
            size: data.len(),
            data,
            stream_index: 1,
            flags: 0,
            duration: 0,
            priv_data: ptr::null_mut(),
            pos: -1,
            codec_id: CodecIDMythTv::Mpeg2Video,
            ty: CodecTypeMythTv::Video,
        }
    }

    #[test]
    fn test_copy_rust_to_c_vector() {
        let mut av_c = empty_c_packet();

        unsafe {
            copy_av_packet_from_rust_to_c(&rust_packet(vec![1, 2, 3, 4, 5]), &mut av_c);

            assert_eq!(av_c.pts, 123456789);
            assert_eq!(av_c.dts, 987654321);
            assert_eq!(av_c.stream_index, 1);
            assert_eq!(av_c.codec_id, CodecIDMythTv::Mpeg2Video.to_c());
            assert_eq!(av_c.type_, CodecTypeMythTv::Video.to_c());
            assert_eq!(av_c.size, 5);
            let data_slice = std::slice::from_raw_parts(av_c.data, av_c.size as usize);
            assert_eq!(data_slice, &[1, 2, 3, 4, 5]);

            ffi_alloc::c_free(av_c.data);
        }
    }

    #[test]
    fn test_copy_rust_to_c_empty_data() {
        let mut av_c = empty_c_packet();

        unsafe {
            copy_av_packet_from_rust_to_c(&rust_packet(Vec::new()), &mut av_c);

            assert_eq!(av_c.size, 0);
            assert_eq!(av_c.pts, 123456789);

            ffi_alloc::c_free(av_c.data);
        }
    }

    #[test]
    fn test_copy_rust_to_c_reuses_c_buffer() {
        let mut av_c = empty_c_packet();

        unsafe {
            copy_av_packet_from_rust_to_c(&rust_packet(vec![1; 16]), &mut av_c);
            copy_av_packet_from_rust_to_c(&rust_packet(vec![7, 8, 9]), &mut av_c);

            assert_eq!(av_c.size, 3);
            let data_slice = std::slice::from_raw_parts(av_c.data, av_c.size as usize);
            assert_eq!(data_slice, &[7, 8, 9]);

            ffi_alloc::c_free(av_c.data);
        }
    }

    #[test]
    fn test_read_packet_rejects_null_pointers() {
        let mut header_state: c_uint = 0;
        let mut psm_es_type = [0u8; 256];
        let mut av_c = empty_c_packet();

        let result = unsafe {
            ccxr_mpegps_read_packet(
                ptr::null_mut(),
                ptr::null_mut(),
                &mut header_state,
                psm_es_type.as_mut_ptr(),
                &mut av_c,
            )
        };
        assert_eq!(result, -1);
    }
}
