use crate::bindings::{lib_cc_decode, lib_ccx_ctx, list_head};
use std::ptr::null_mut;
// HList (Hyperlinked List)

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
#[cfg(test)]
mod tests {
    use super::*;
    use crate::demuxer::common_types::CapInfo;
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
}
