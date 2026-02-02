use std::os::raw::c_void;

extern "C" {
    fn free(ptr: *mut c_void);
}
/// Frees memory allocated by C code (malloc/calloc)

#[inline]
pub unsafe fn c_free<T>(ptr: *mut T) {
    if !ptr.is_null() {
        free(ptr as *mut c_void);
    }
}
