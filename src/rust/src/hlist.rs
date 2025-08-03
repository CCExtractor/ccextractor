use crate::bindings::{
    cap_info, ccx_code_type_CCX_CODEC_NONE, ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM,
    demuxer_data, dvbsub_init_decoder, lib_cc_decode, lib_ccx_ctx, list_head, telxcc_init,
};
use crate::common::CType;
use crate::demuxer::common_structs::CcxDemuxer;
use crate::transportstream::core::copy_capbuf_demux_data;
use lib_ccxr::common::{Codec, Options, StreamType};
use std::ffi::c_int;
use std::os::raw::c_void;
use std::ptr::null_mut;
// HList (Hyperlinked List)
/* Note
We are using demuxer_data and cap_info from lib_ccx from bindings,
because data.next_stream and cinfo.all_stream(and others) are linked lists,
and are impossible to copy to rust with good performance in mind.
So, when the core C library is made into rust, please make sure to update these structs
to DemuxerData and CapInfo, respectively.
*/
#[allow(clippy::ptr_eq)]
pub fn list_empty(head: &list_head) -> bool {
    head.next == head as *const list_head as *mut list_head
}

pub fn list_entry<T>(ptr: *mut list_head, offset: usize) -> *mut T {
    (ptr as *mut u8).wrapping_sub(offset) as *mut T
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers.
pub unsafe fn list_del(entry: &mut list_head) {
    if entry.prev.is_null() && entry.next.is_null() {
        return;
    }

    if !entry.prev.is_null() {
        (*entry.prev).next = entry.next;
    }
    if !entry.next.is_null() {
        (*entry.next).prev = entry.prev;
    }

    entry.next = null_mut();
    entry.prev = null_mut();
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers.
pub unsafe fn list_add(new_entry: &mut list_head, head: &mut list_head) {
    // Link new entry between head and head.next
    new_entry.next = head.next;
    new_entry.prev = head as *mut list_head;

    // Update existing nodes' links
    if !head.next.is_null() {
        (*head.next).prev = new_entry as *mut list_head;
    }

    // Finalize head's new link
    head.next = new_entry as *mut list_head;
}
pub fn init_list_head(head: &mut list_head) {
    head.next = head as *mut list_head;
    head.prev = head as *mut list_head;
}

// Helper function to calculate offset of a field in a struct
// This is similar to ccx_offsetof macro in C
macro_rules! offset_of {
    ($ty:ty, $field:ident) => {{
        let dummy = std::mem::MaybeUninit::<$ty>::uninit();
        let dummy_ptr = dummy.as_ptr();
        let field_ptr = std::ptr::addr_of!((*dummy_ptr).$field);
        (field_ptr as *const u8).offset_from(dummy_ptr as *const u8) as usize
    }};
}

// Container_of equivalent - gets the containing struct from a member pointer
/// # Safety
/// This function is unsafe because we have to add a signed address to a raw pointer,
unsafe fn container_of<T>(ptr: *const list_head, offset: usize) -> *mut T {
    (ptr as *const u8).offset(-(offset as isize)) as *mut T
}

// Iterator for list_for_each_entry functionality
pub struct ListIterator<T> {
    current: *mut list_head,
    head: *mut list_head,
    offset: usize,
    _phantom: std::marker::PhantomData<T>,
}

impl<T> ListIterator<T> {
    /// # Safety
    /// This function is unsafe because it dereferences raw pointers.
    pub unsafe fn new(head: *mut list_head, offset: usize) -> Self {
        Self {
            current: (*head).next,
            head,
            offset,
            _phantom: std::marker::PhantomData,
        }
    }
}

impl<T> Iterator for ListIterator<T> {
    type Item = *mut T;

    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            if self.current == self.head {
                None
            } else {
                let entry = container_of::<T>(self.current, self.offset);
                self.current = (*self.current).next;
                Some(entry)
            }
        }
    }
}

// Main macro that replaces list_for_each_entry
#[macro_export]
macro_rules! list_for_each_entry {
    ($head:expr, $ty:ty, $member:ident) => {{
        ListIterator::<$ty>::new($head, offset_of!($ty, $member))
    }};
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
pub unsafe fn is_decoder_processed_enough(ctx: &mut lib_ccx_ctx) -> i32 {
    {
        for dec_ctx in list_for_each_entry!(&mut ctx.dec_ctx_head, lib_cc_decode, list) {
            unsafe {
                if (*dec_ctx).processed_enough == 1 && ctx.multiprogram == 0 {
                    return 1;
                }
            }
        }
    }

    0
}

/// # Safety
/// This function is unsafe because it calls list_del.
pub unsafe fn dinit_cap(cinfo_tree: &mut cap_info) {
    // Calculate offset of all_stream within CapInfo
    let offset = {
        let dummy = cap_info {
            pid: 0,
            all_stream: list_head::default(),
            sib_head: list_head::default(),
            sib_stream: list_head::default(),
            pg_stream: list_head::default(),
            ..Default::default()
        };
        &dummy.all_stream as *const list_head as usize - &dummy as *const cap_info as usize
    };

    // Process all_stream list
    while !list_empty(&cinfo_tree.all_stream) {
        let current = cinfo_tree.all_stream.next;
        let entry = list_entry::<cap_info>(current, offset);

        // Remove from list before processing
        if let Some(current) = current.as_mut() {
            list_del(current);
        }
        // Free resources
        let _ = &mut (*entry).capbuf;
        freep(&mut (*entry).codec_private_data);
        let _ = Box::from_raw(entry);
    }

    // Reinitialize all relevant list heads
    init_list_head(&mut cinfo_tree.all_stream);
    init_list_head(&mut cinfo_tree.sib_stream);
    init_list_head(&mut cinfo_tree.pg_stream);
}

pub fn freep<T>(ptr: &mut *mut T) {
    unsafe {
        if !ptr.is_null() {
            let _ = Box::from_raw(*ptr);
            *ptr = null_mut();
        }
    }
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers.
unsafe fn __list_add(elem: &mut list_head, prev: *mut list_head, next: *mut list_head) {
    (*next).prev = elem as *mut list_head;
    elem.next = next;
    elem.prev = prev;
    (*prev).next = elem as *mut list_head;
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers.
pub unsafe fn list_add_tail(elem: &mut list_head, head: &mut list_head) {
    __list_add(elem, head.prev, head);
}

/// Initialize private data based on codec type
/// # Safety
/// This function is unsafe because it returns raw pointers that need proper management
unsafe fn init_private_data(codec: Codec) -> *mut std::ffi::c_void {
    match codec {
        Codec::Teletext => telxcc_init(),
        Codec::Dvb => dvbsub_init_decoder(null_mut(), 0),
        _ => null_mut(),
    }
}

/// Update capture info for a given PID
/// # Safety
/// This function is unsafe because it manipulates raw pointers and linked lists
#[allow(clippy::too_many_arguments)]
pub unsafe fn update_capinfo(
    ptr: &mut cap_info,
    pid: i32,
    stream: StreamType,
    codec: Option<Codec>,
    pn: i32,
    private_data: *mut c_void,
    flag_ts_forced_cappid: bool,
    ts_datastreamtype: StreamType,
) -> i32 {
    // Check if context is null

    // Check stream type compatibility
    if ts_datastreamtype != StreamType::Unknownstream && ts_datastreamtype != stream {
        return -1;
    }

    // Search for existing entry with same PID
    for tmp_ptr in list_for_each_entry!(&mut ptr.all_stream, cap_info, all_stream) {
        // info!("Checking PID: {}\n", (*tmp_ptr).pid);
        // info!(
        //     "Current Stream: {:?}, Codec: {:?}\n",
        //     (*tmp_ptr).stream,
        //     (*tmp_ptr).codec
        // );

        let tmp = &mut *tmp_ptr;

        if tmp.pid == pid {
            // Update existing entry if it has unknown stream or no codec
            if tmp.stream == ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM
                || tmp.codec == ccx_code_type_CCX_CODEC_NONE
            {
                if stream != StreamType::Unknownstream {
                    tmp.stream = stream.to_ctype() as _;
                }

                if codec.is_some() {
                    tmp.codec = codec.unwrap().to_ctype();
                    tmp.codec_private_data = init_private_data(codec.unwrap());
                }

                tmp.saw_pesstart = 0;
                tmp.capbuflen = 0;
                tmp.capbufsize = 0;
                tmp.ignore = 0;

                if !private_data.is_null() {
                    tmp.codec_private_data = private_data;
                }
            }
            return 0;
        }
    }

    // Check if forced cap PID is enabled
    if flag_ts_forced_cappid {
        return -102;
    }

    // Allocate new cap_info structure using Box
    let tmp_box = Box::new(cap_info {
        pid: 0,
        stream: ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM,
        codec: ccx_code_type_CCX_CODEC_NONE,
        program_number: 0,
        saw_pesstart: 0,
        capbuflen: 0,
        capbufsize: 0,
        capbuf: null_mut(),
        ignore: 0,
        codec_private_data: null_mut(),
        all_stream: list_head {
            next: null_mut(),
            prev: null_mut(),
        },
        pg_stream: list_head {
            next: null_mut(),
            prev: null_mut(),
        },
        sib_stream: list_head {
            next: null_mut(),
            prev: null_mut(),
        },
        sib_head: list_head {
            next: null_mut(),
            prev: null_mut(),
        },
        prev_counter: 0,
    });
    let tmp_ptr = Box::into_raw(tmp_box);

    let tmp = &mut *tmp_ptr;

    // Initialize new entry
    tmp.pid = pid;
    tmp.stream = stream.to_ctype() as _;
    if codec.is_none() {
        tmp.codec = ccx_code_type_CCX_CODEC_NONE;
    } else {
        tmp.codec = codec.unwrap().to_ctype();
    }
    tmp.program_number = pn;
    tmp.saw_pesstart = 0;
    tmp.capbuflen = 0;
    tmp.capbufsize = 0;
    tmp.capbuf = null_mut();
    tmp.ignore = 0;

    // Set private data
    if private_data.is_null() && codec.is_some() {
        tmp.codec_private_data = init_private_data(codec.unwrap());
    } else {
        tmp.codec_private_data = private_data;
    }

    // Add to all_stream list
    list_add_tail(&mut tmp.all_stream, &mut ptr.all_stream);

    // Find program with matching program number
    for program_iter_ptr in list_for_each_entry!(&mut ptr.pg_stream, cap_info, pg_stream) {
        let program_iter = &mut *program_iter_ptr;

        if program_iter.program_number == pn {
            list_add_tail(&mut tmp.sib_stream, &mut program_iter.sib_head);
            return 0;
        }
    }

    // No matching program found, create new program entry
    init_list_head(&mut tmp.sib_head);
    list_add_tail(&mut tmp.sib_stream, &mut tmp.sib_head);
    list_add_tail(&mut tmp.pg_stream, &mut ptr.pg_stream);

    0
}

/// Check if capture info is needed for a given PID
/// # Safety
/// This function is unsafe because it dereferences raw pointers in the linked list
pub unsafe fn need_cap_info_for_pid(cap: &mut cap_info, pid: i32) -> bool {
    if list_empty(&cap.all_stream) {
        return false;
    }

    let head_ptr = &cap.all_stream as *const list_head as *mut list_head;
    for iter in list_for_each_entry!(head_ptr, cap_info, all_stream) {
        if (*iter).pid == pid && (*iter).stream == StreamType::Unknownstream.to_ctype() {
            return true;
        }
    }
    false
}
/// Get capture info for a given PID
/// # Safety
/// This function is unsafe because it dereferences raw pointers in the linked list
pub unsafe fn get_cinfo(cinfo_tree: &mut cap_info, pid: i32) -> *mut cap_info {
    let head_ptr = &cinfo_tree.all_stream as *const list_head as *mut list_head;

    for iter in list_for_each_entry!(head_ptr, cap_info, all_stream) {
        if (*iter).pid == pid
            && (*iter).codec != ccx_code_type_CCX_CODEC_NONE
            && (*iter).stream != ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM
        {
            return iter;
        }
    }

    null_mut()
}
/// Process all cap_info entries and copy their buffers to demuxer data
/// # Safety
/// This function is unsafe because it dereferences raw pointers in the linked list
pub unsafe fn cinfo_cremation(
    ctx: &mut CcxDemuxer,
    data: *mut *mut demuxer_data,
    cinfo_tree: &mut cap_info,
    ccx_options: &mut Options,
    haup_capbuf: *mut u8,
    haup_capbuflen: &mut c_int,
    last_pts: &mut u64,
) {
    let head_ptr = &cinfo_tree.all_stream as *const list_head as *mut list_head;

    for iter in list_for_each_entry!(head_ptr, cap_info, all_stream) {
        copy_capbuf_demux_data(
            ctx,
            data,
            &mut *iter,
            ccx_options,
            haup_capbuf,
            &mut *haup_capbuflen, // Dereference the pointer to get a mutable reference
            &mut *last_pts,
        );
        freep(&mut (*iter).capbuf);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bindings::{
        ccx_code_type_CCX_CODEC_NONE, ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM,
    };
    use crate::demuxer::common_structs::CapInfo;
    use std::ptr;

    pub fn create_capinfo() -> *mut CapInfo {
        Box::into_raw(Box::new(CapInfo {
            all_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            capbuf: Box::into_raw(Box::new(0u8)),
            ..Default::default()
        }))
    }
    #[test]
    fn test_list_operations() {
        let mut head = list_head {
            next: null_mut(),
            prev: null_mut(),
        };
        crate::hlist::init_list_head(&mut head);
        assert!(list_empty(&mut head));

        // Test list insertion/deletion
        unsafe {
            let cap = create_capinfo();
            crate::hlist::list_add(&mut (*cap).all_stream, &mut head);
            assert!(!list_empty(&mut head));

            list_del(&mut (*cap).all_stream);
            assert!(list_empty(&mut head));

            let _ = Box::from_raw(cap);
        }
    }

    #[test]
    fn test_list_add() {
        let mut head = list_head {
            next: null_mut(),
            prev: null_mut(),
        };
        crate::hlist::init_list_head(&mut head);

        unsafe {
            let mut entry1 = list_head {
                next: null_mut(),
                prev: null_mut(),
            };
            let mut entry2 = list_head {
                next: null_mut(),
                prev: null_mut(),
            };

            crate::hlist::list_add(&mut entry1, &mut head);
            assert_eq!(head.next, &mut entry1 as *mut list_head);
            assert_eq!(entry1.prev, &mut head as *mut list_head);
            assert_eq!(entry1.next, &mut head as *mut list_head);

            crate::hlist::list_add(&mut entry2, &mut head);
            assert_eq!(head.next, &mut entry2 as *mut list_head);
            assert_eq!(entry2.prev, &mut head as *mut list_head);
            assert_eq!(entry2.next, &mut entry1 as *mut list_head);
            assert_eq!(entry1.prev, &mut entry2 as *mut list_head);
        }
    }

    // Helper function to initialize a list_head as empty (circular)
    /// # Safety
    /// This function is unsafe because it dereferences raw pointers.
    unsafe fn init_list_head(head: *mut list_head) {
        (*head).next = head;
        (*head).prev = head;
    }

    // Helper function to add an entry after the head
    /// # Safety
    /// This function is unsafe because it dereferences raw pointers.
    unsafe fn list_add(new_entry: *mut list_head, head: *mut list_head) {
        let next = (*head).next;
        (*new_entry).next = next;
        (*new_entry).prev = head;
        (*next).prev = new_entry;
        (*head).next = new_entry;
    }

    #[test]
    fn test_offset_of_macro() {
        // Test that offset_of calculates correct field offsets
        unsafe {
            let offset_processed = offset_of!(lib_cc_decode, processed_enough);

            // processed_enough should be after list_head (which contains 2 pointers)
            assert!(offset_processed > 0);
            assert!(offset_processed >= std::mem::size_of::<list_head>());
        }
    }

    #[test]
    fn test_empty_list_iteration() {
        let mut head = list_head {
            next: ptr::null_mut(),
            prev: ptr::null_mut(),
        };
        unsafe {
            init_list_head(&mut head);
        }
        let mut count = 0;
        unsafe {
            for _entry in list_for_each_entry!(&mut head, lib_cc_decode, list) {
                count += 1;
            }
        }

        assert_eq!(count, 0, "Empty list should yield no entries");
    }

    #[test]
    fn test_single_entry_list() {
        let mut head = list_head {
            next: ptr::null_mut(),
            prev: ptr::null_mut(),
        };
        unsafe {
            init_list_head(&mut head);

            let mut entry1 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 42,
                ..Default::default()
            };

            list_add(&mut entry1.list, &mut head);
        }

        let mut count = 0;
        let mut found_value = 0;

        unsafe {
            for entry in list_for_each_entry!(&mut head, lib_cc_decode, list) {
                count += 1;
                found_value = (*entry).processed_enough;
            }
        }
        assert_eq!(count, 1, "Single entry list should yield exactly one entry");
        assert_eq!(found_value, 42, "Should find the correct entry value");
    }

    #[test]
    fn test_multiple_entries_list() {
        let mut head = list_head {
            next: ptr::null_mut(),
            prev: ptr::null_mut(),
        };
        unsafe {
            init_list_head(&mut head);
        }
        let mut entry1 = lib_cc_decode {
            list: list_head {
                next: ptr::null_mut(),
                prev: ptr::null_mut(),
            },
            processed_enough: 10,
            ..Default::default()
        };

        let mut entry2 = lib_cc_decode {
            list: list_head {
                next: ptr::null_mut(),
                prev: ptr::null_mut(),
            },
            processed_enough: 20,
            ..Default::default()
        };

        let mut entry3 = lib_cc_decode {
            list: list_head {
                next: ptr::null_mut(),
                prev: ptr::null_mut(),
            },
            processed_enough: 30,
            ..Default::default()
        };

        // Add entries in reverse order to test proper iteration
        unsafe {
            list_add(&mut entry3.list, &mut head);
            list_add(&mut entry2.list, &mut head);
            list_add(&mut entry1.list, &mut head);
        }
        let mut count = 0;
        let mut values = Vec::new();

        unsafe {
            for entry in list_for_each_entry!(&mut head, lib_cc_decode, list) {
                count += 1;
                values.push((*entry).processed_enough);
            }
        }
        assert_eq!(count, 3, "Should iterate over all three entries");
        // Entries should be found in the order they were added (LIFO due to list_add)
        assert_eq!(values, vec![10, 20, 30], "Should iterate in correct order");
    }

    #[test]
    fn test_is_decoder_processed_enough_empty_list() {
        unsafe {
            let mut ctx = lib_ccx_ctx {
                dec_ctx_head: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                multiprogram: 0,
                ..Default::default()
            };
            init_list_head(&mut ctx.dec_ctx_head);

            let result = is_decoder_processed_enough(&mut ctx);
            assert_eq!(result, 0, "Empty list should return false (0)");
        }
    }

    #[test]
    fn test_is_decoder_processed_enough_no_processed_entries() {
        unsafe {
            let mut ctx = lib_ccx_ctx {
                dec_ctx_head: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                multiprogram: 0,
                ..Default::default()
            };
            init_list_head(&mut ctx.dec_ctx_head);

            let mut entry1 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 0, // Not processed
                ..Default::default()
            };

            let mut entry2 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 0, // Not processed
                ..Default::default()
            };

            list_add(&mut entry1.list, &mut ctx.dec_ctx_head);
            list_add(&mut entry2.list, &mut ctx.dec_ctx_head);

            let result = is_decoder_processed_enough(&mut ctx);
            assert_eq!(result, 0, "No processed entries should return false (0)");
        }
    }

    #[test]
    fn test_is_decoder_processed_enough_with_processed_entry() {
        unsafe {
            let mut ctx = lib_ccx_ctx {
                dec_ctx_head: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                multiprogram: 0, // Single program mode
                ..Default::default()
            };
            init_list_head(&mut ctx.dec_ctx_head);

            let mut entry1 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 0, // Not processed
                ..Default::default()
            };

            let mut entry2 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 1, // Processed
                ..Default::default()
            };

            list_add(&mut entry1.list, &mut ctx.dec_ctx_head);
            list_add(&mut entry2.list, &mut ctx.dec_ctx_head);

            let result = is_decoder_processed_enough(&mut ctx);
            assert_eq!(
                result, 1,
                "Should return true (1) when entry is processed and single program"
            );
        }
    }

    #[test]
    fn test_is_decoder_processed_enough_multiprogram_mode() {
        unsafe {
            let mut ctx = lib_ccx_ctx {
                dec_ctx_head: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                multiprogram: 1, // Multi program mode
                ..Default::default()
            };
            init_list_head(&mut ctx.dec_ctx_head);

            let mut entry = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 1, // Processed
                ..Default::default()
            };

            list_add(&mut entry.list, &mut ctx.dec_ctx_head);

            let result = is_decoder_processed_enough(&mut ctx);
            assert_eq!(
                result, 0,
                "Should return false (0) in multiprogram mode even if processed"
            );
        }
    }

    #[test]
    fn test_is_decoder_processed_enough_early_return() {
        unsafe {
            let mut ctx = lib_ccx_ctx {
                dec_ctx_head: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                multiprogram: 0,
                ..Default::default()
            };
            init_list_head(&mut ctx.dec_ctx_head);

            let mut entry1 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 1, // Processed - should cause early return
                ..Default::default()
            };

            let mut entry2 = lib_cc_decode {
                list: list_head {
                    next: ptr::null_mut(),
                    prev: ptr::null_mut(),
                },
                processed_enough: 0, // This shouldn't be reached
                ..Default::default()
            };

            list_add(&mut entry2.list, &mut ctx.dec_ctx_head);
            list_add(&mut entry1.list, &mut ctx.dec_ctx_head);

            let result = is_decoder_processed_enough(&mut ctx);
            assert_eq!(result, 1, "Should return true (1) on first processed entry");
        }
    }
    fn create_test_capinfo() -> *mut cap_info {
        Box::into_raw(Box::new(cap_info {
            pid: 123,
            program_number: 1,
            stream: ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM,
            codec: ccx_code_type_CCX_CODEC_NONE,
            capbuf: Box::into_raw(Box::new(0u8)),
            capbufsize: 1024,
            capbuflen: 0,
            saw_pesstart: 0,
            prev_counter: 0,
            codec_private_data: null_mut(),
            ignore: 0,
            all_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_head: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            pg_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
        }))
    }
    #[test]
    fn test_dinit_cap_safety() {
        let mut cinfo_tree = Box::new(cap_info {
            pid: 0,
            program_number: 0,
            stream: ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM,
            codec: ccx_code_type_CCX_CODEC_NONE,
            capbufsize: 0,
            capbuf: null_mut(),
            capbuflen: 0,
            saw_pesstart: 0,
            prev_counter: 0,
            codec_private_data: null_mut(),
            ignore: 0,
            all_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_head: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            pg_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
        });

        // Properly initialize list heads
        crate::hlist::init_list_head(&mut cinfo_tree.all_stream);
        crate::hlist::init_list_head(&mut cinfo_tree.sib_stream);
        crate::hlist::init_list_head(&mut cinfo_tree.pg_stream);

        unsafe {
            // Add test entries
            let cap1 = create_test_capinfo();
            crate::hlist::list_add(&mut (*cap1).all_stream, &mut cinfo_tree.all_stream);

            let cap2 = create_test_capinfo();
            crate::hlist::list_add(&mut (*cap2).all_stream, &mut cinfo_tree.all_stream);

            // Convert to raw pointer for demuxer
            let ctx_ptr = Box::into_raw(cinfo_tree);

            dinit_cap(&mut *ctx_ptr);

            // Verify cleanup
            assert!(list_empty(&(*ctx_ptr).all_stream));
            assert_eq!(
                (*ctx_ptr).all_stream.next,
                &mut (*ctx_ptr).all_stream as *mut list_head
            );

            // Cleanup context
            let _ = Box::from_raw(ctx_ptr);
        }
    }
}
