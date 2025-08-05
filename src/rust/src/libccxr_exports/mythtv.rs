use crate::bindings::ccx_demuxer;
use crate::bindings::AVPacketMythTV;
use crate::ccx_options;
use crate::common::{copy_from_rust, copy_to_rust};
use crate::libccxr_exports::demuxer::{copy_demuxer_from_c_to_rust, copy_demuxer_from_rust_to_c};
use crate::mythtv::common_structs::{AVPacketMythTv, CodecIDMythTv, CodecTypeMythTv};
use crate::mythtv::myth::mpegps_read_packet;
use lib_ccxr::common::Options;
use std::os::raw::{c_int, c_uchar, c_uint};
/// # Safety
/// This function assumes that the pointer `av` is valid and points to a properly allocated `AVPacket` structure.
/// Caller must ensure that the memory pointed to by `av` is valid for the lifetime of the returned `AVPacketMythTv`.
pub unsafe fn copy_av_packet_from_c_to_rust(av: *mut AVPacketMythTV) -> AVPacketMythTv {
    if av.is_null() {
        panic!("Null AVPacket pointer passed to copy_av_packet_from_c_to_rust");
    }
    let av_packet = &*av;
    let data = if av_packet.data.is_null() {
        Vec::new()
    } else {
        // FIXED: Copy the data instead of taking ownership
        let slice = std::slice::from_raw_parts(av_packet.data, av_packet.size as usize);
        slice.to_vec()
    };
    AVPacketMythTv {
        pts: av_packet.pts,
        dts: av_packet.dts,
        data,
        size: av_packet.size as usize,
        stream_index: av_packet.stream_index,
        flags: av_packet.flags,
        duration: av_packet.duration,
        priv_data: av_packet.priv_,
        pos: av_packet.pos,
        codec_id: CodecIDMythTv::from_c(av_packet.codec_id).unwrap_or(CodecIDMythTv::None),
        ty: CodecTypeMythTv::from_c(av_packet.type_).unwrap_or(CodecTypeMythTv::Unknown),
    }
}
/// # Safety
/// This function assumes that the pointer `av_rust` is valid and points to a properly initialized `AVPacketMythTv` structure.
/// The caller must ensure that the memory pointed to by `av_c` is valid and allocated for an `AVPacket`.
pub unsafe fn copy_av_packet_from_rust_to_c(
    av_rust: &mut AVPacketMythTv,
    av_c: *mut AVPacketMythTV,
) {
    if av_c.is_null() {
        panic!("Null AVPacket pointer passed to copy_av_packet_from_rust_to_c");
    }

    let av_packet = &mut *av_c;

    // Copy scalar fields
    av_packet.pts = av_rust.pts;
    av_packet.dts = av_rust.dts;
    av_packet.stream_index = av_rust.stream_index;
    av_packet.flags = av_rust.flags;
    av_packet.duration = av_rust.duration;
    av_packet.priv_ = av_rust.priv_data;
    av_packet.pos = av_rust.pos;
    av_packet.codec_id = av_rust.codec_id.to_c();
    av_packet.type_ = av_rust.ty.to_c();

    // Handle data field - need to allocate C memory
    if av_rust.data.is_empty() {
        av_packet.data = std::ptr::null_mut();
        av_packet.size = 0;
    } else {
        // Allocate C memory for the data using std::alloc
        let size = av_rust.data.len();
        let layout = std::alloc::Layout::from_size_align(size, 1).unwrap();
        let c_data = std::alloc::alloc(layout);

        if c_data.is_null() {
            panic!("Failed to allocate memory for AVPacket data");
        }

        // Copy the data from Rust Vec to C memory
        std::ptr::copy_nonoverlapping(av_rust.data.as_ptr(), c_data, size);

        av_packet.data = c_data;
        av_packet.size = size as i32;
    }

    // Note: The caller is responsible for freeing av_packet.data using std::alloc::dealloc()
}
/// # Safety
/// This function assumes that the pointer `ctx` is valid and points to a properly allocated `ccx_demuxer` structure.
/// The caller must ensure that the memory pointed to by `ctx` is valid for the lifetime of the demuxer.
/// It also assumes that `header_state` and `psm_es_type` are valid mutable references.
#[no_mangle]
pub unsafe fn ccxr_mpegps_read_packet(
    ctx: *mut ccx_demuxer,
    header_state: &mut c_uint,
    psm_es_type: &mut [c_uchar; 256],
    av: *mut AVPacketMythTV,
) -> c_int {
    if ctx.is_null() {
        return -1; // Error: null context
    }
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);
    let mut av_packet = copy_av_packet_from_c_to_rust(av);
    let result = mpegps_read_packet(
        &mut demux_ctx,
        header_state,
        psm_es_type,
        &mut CcxOptions,
        &mut av_packet,
    );
    copy_av_packet_from_rust_to_c(&mut av_packet, av);
    copy_demuxer_from_rust_to_c(ctx, &demux_ctx);
    copy_from_rust(&raw mut ccx_options, CcxOptions);
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr;

    #[test]
    fn test_copy_c_to_rust_vector() {
        // Create test data that will persist for the duration of the test
        let mut test_data = vec![1, 2, 3, 4, 5];

        let mut av_packet = AVPacketMythTV {
            pts: 123456789,
            dts: 987654321,
            data: test_data.as_mut_ptr(),
            size: test_data.len() as i32,
            stream_index: 1,
            flags: 0,
            duration: 0,
            destruct: None,
            priv_: ptr::null_mut(),
            pos: -1,
            codec_id: 0,
            type_: 0,
        };

        let av_packet_rust = unsafe { copy_av_packet_from_c_to_rust(&mut av_packet) };
        assert_eq!(av_packet_rust.data, vec![1, 2, 3, 4, 5]);

        // test_data is still owned by this scope and will be properly freed
    }

    #[test]
    fn test_copy_c_to_rust_null_data() {
        let mut av_packet = AVPacketMythTV {
            pts: 123456789,
            dts: 987654321,
            data: ptr::null_mut(),
            size: 0,
            stream_index: 1,
            flags: 0,
            duration: 0,
            destruct: None,
            priv_: ptr::null_mut(),
            pos: -1,
            codec_id: 0,
            type_: 0,
        };

        let av_packet_rust = unsafe { copy_av_packet_from_c_to_rust(&mut av_packet) };
        assert_eq!(av_packet_rust.data, Vec::<u8>::new());
    }
    #[test]
    fn test_copy_rust_to_c_vector() {
        let mut av_rust = AVPacketMythTv {
            pts: 123456789,
            dts: 987654321,
            data: vec![1, 2, 3, 4, 5],
            size: 5,
            stream_index: 1,
            flags: 0,
            duration: 0,
            priv_data: ptr::null_mut(),
            pos: -1,
            codec_id: CodecIDMythTv::None,
            ty: CodecTypeMythTv::Unknown,
        };

        let mut av_c = AVPacketMythTV {
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
        };

        unsafe {
            copy_av_packet_from_rust_to_c(&mut av_rust, &mut av_c);

            // Check scalar fields
            assert_eq!(av_c.pts, 123456789);
            assert_eq!(av_c.dts, 987654321);
            assert_eq!(av_c.stream_index, 1);
            assert_eq!(av_c.size, 5);

            // Check data
            assert!(!av_c.data.is_null());
            let data_slice = std::slice::from_raw_parts(av_c.data, av_c.size as usize);
            assert_eq!(data_slice, &[1, 2, 3, 4, 5]);

            // Clean up allocated memory
            let layout = std::alloc::Layout::from_size_align(av_c.size as usize, 1).unwrap();
            std::alloc::dealloc(av_c.data, layout);
        }
    }

    #[test]
    fn test_copy_rust_to_c_empty_data() {
        let mut av_rust = AVPacketMythTv {
            pts: 123456789,
            dts: 987654321,
            data: Vec::new(),
            size: 0,
            stream_index: 1,
            flags: 0,
            duration: 0,
            priv_data: ptr::null_mut(),
            pos: -1,
            codec_id: CodecIDMythTv::None,
            ty: CodecTypeMythTv::Unknown,
        };

        let mut av_c = AVPacketMythTV {
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
        };

        unsafe {
            copy_av_packet_from_rust_to_c(&mut av_rust, &mut av_c);

            // Check that empty data is handled correctly
            assert!(av_c.data.is_null());
            assert_eq!(av_c.size, 0);
            assert_eq!(av_c.pts, 123456789);
        }
    }

    #[test]
    fn test_round_trip_conversion() {
        // Test C -> Rust -> C conversion
        let mut original_data = vec![10, 20, 30, 40, 50];
        let mut av_c_original = AVPacketMythTV {
            pts: 999888777,
            dts: 777888999,
            data: original_data.as_mut_ptr(),
            size: original_data.len() as i32,
            stream_index: 2,
            flags: 1,
            duration: 100,
            destruct: None,
            priv_: ptr::null_mut(),
            pos: 12345,
            codec_id: 86,
            type_: 1,
        };

        unsafe {
            // C -> Rust
            let av_rust = copy_av_packet_from_c_to_rust(&mut av_c_original);

            // Rust -> C
            let mut av_c_new = AVPacketMythTV {
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
            };

            let mut av_rust_mut = av_rust;
            copy_av_packet_from_rust_to_c(&mut av_rust_mut, &mut av_c_new);

            // Verify all fields match
            assert_eq!(av_c_new.pts, av_c_original.pts);
            assert_eq!(av_c_new.dts, av_c_original.dts);
            assert_eq!(av_c_new.stream_index, av_c_original.stream_index);
            assert_eq!(av_c_new.flags, av_c_original.flags);
            assert_eq!(av_c_new.duration, av_c_original.duration);
            assert_eq!(av_c_new.pos, av_c_original.pos);
            assert_eq!(av_c_new.codec_id, av_c_original.codec_id);
            assert_eq!(av_c_new.type_, av_c_original.type_);
            assert_eq!(av_c_new.size, av_c_original.size);

            // Verify data content
            let new_data_slice = std::slice::from_raw_parts(av_c_new.data, av_c_new.size as usize);
            assert_eq!(new_data_slice, &[10, 20, 30, 40, 50]);

            // Clean up
            let layout = std::alloc::Layout::from_size_align(av_c_new.size as usize, 1).unwrap();
            std::alloc::dealloc(av_c_new.data, layout);
        }
    }
}
