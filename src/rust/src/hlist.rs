use crate::bindings::list_head;
use std::ptr::null_mut;
// HList (Hyperlinked List)

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

mod tests {
    use super::*;
    use crate::demuxer::demux::create_capinfo;

    #[test]
    fn test_list_operations() {
        let mut head = list_head {
            next: null_mut(),
            prev: null_mut(),
        };
        init_list_head(&mut head);
        assert!(list_empty(&mut head));

        // Test list insertion/deletion
        unsafe {
            let cap = create_capinfo();
            list_add(&mut (*cap).all_stream, &mut head);
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
        init_list_head(&mut head);

        unsafe {
            let mut entry1 = list_head {
                next: null_mut(),
                prev: null_mut(),
            };
            let mut entry2 = list_head {
                next: null_mut(),
                prev: null_mut(),
            };

            list_add(&mut entry1, &mut head);
            assert_eq!(head.next, &mut entry1 as *mut list_head);
            assert_eq!(entry1.prev, &mut head as *mut list_head);
            assert_eq!(entry1.next, &mut head as *mut list_head);

            list_add(&mut entry2, &mut head);
            assert_eq!(head.next, &mut entry2 as *mut list_head);
            assert_eq!(entry2.prev, &mut head as *mut list_head);
            assert_eq!(entry2.next, &mut entry1 as *mut list_head);
            assert_eq!(entry1.prev, &mut entry2 as *mut list_head);
        }
    }
}
