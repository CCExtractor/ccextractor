use std::os::raw::c_void;

extern "C" {
    fn free(ptr: *mut c_void);
}
/// Frees memory allocated by C code (malloc/calloc)
///
/// # Safety
///
/// The pointer must have been allocated by C's malloc/calloc/realloc and
/// not yet freed. Calling this function with a pointer that was not
/// allocated by the C allocator is undefined behavior.
#[inline]
pub unsafe fn c_free<T>(ptr: *mut T) {
    if !ptr.is_null() {
        free(ptr as *mut c_void);
    }
}
