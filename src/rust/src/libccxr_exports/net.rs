use crate::bindings::*;

use std::convert::TryInto;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_uchar, c_uint, c_void};

use lib_ccxr::util::net::c_functions::*;

/// Rust equivalent for `connect_to_srv` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `addr` must not be null. All the strings must end with a nul character.
#[no_mangle]
pub unsafe extern "C" fn ccxr_connect_to_srv(
    addr: *const c_char,
    port: *const c_char,
    cc_desc: *const c_char,
    pwd: *const c_char,
) {
    let addr = CStr::from_ptr(addr).to_str().unwrap();

    let port = if !port.is_null() {
        Some(CStr::from_ptr(port).to_str().unwrap().parse().unwrap())
    } else {
        None
    };

    let cc_desc = if !cc_desc.is_null() {
        Some(CStr::from_ptr(cc_desc).to_str().unwrap())
    } else {
        None
    };

    let pwd = if !pwd.is_null() {
        Some(CStr::from_ptr(pwd).to_str().unwrap())
    } else {
        None
    };

    connect_to_srv(addr, port, cc_desc, pwd);
}

/// Rust equivalent for `net_send_header` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `data` must not be null and should have a length of `len`.
/// [`ccxr_connect_to_srv`] or `connect_to_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_send_header(data: *const c_uchar, len: usize) {
    let buffer = std::slice::from_raw_parts(data, len);
    net_send_header(buffer);
}

/// Rust equivalent for `net_send_cc` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `data` must not be null and should have a length of `len`.
/// [`ccxr_connect_to_srv`] or `connect_to_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_send_cc(
    data: *const c_uchar,
    len: usize,
    _private_data: *const c_void,
    _sub: *const cc_subtitle,
) -> c_int {
    let buffer = std::slice::from_raw_parts(data, len);
    if net_send_cc(buffer) {
        1
    } else {
        -1
    }
}

/// Rust equivalent for `net_check_conn` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// [`ccxr_connect_to_srv`] or `connect_to_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_check_conn() {
    net_check_conn()
}

/// Rust equivalent for `net_send_epg` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `start` and `stop` must not be null. All the strings must end with a nul character.
/// [`ccxr_connect_to_srv`] or `connect_to_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_send_epg(
    start: *const c_char,
    stop: *const c_char,
    title: *const c_char,
    desc: *const c_char,
    lang: *const c_char,
    category: *const c_char,
) {
    let start = CStr::from_ptr(start).to_str().unwrap();
    let stop = CStr::from_ptr(stop).to_str().unwrap();

    let title = if !title.is_null() {
        Some(CStr::from_ptr(title).to_str().unwrap())
    } else {
        None
    };

    let desc = if !desc.is_null() {
        Some(CStr::from_ptr(desc).to_str().unwrap())
    } else {
        None
    };

    let lang = if !lang.is_null() {
        Some(CStr::from_ptr(lang).to_str().unwrap())
    } else {
        None
    };

    let category = if !category.is_null() {
        Some(CStr::from_ptr(category).to_str().unwrap())
    } else {
        None
    };

    net_send_epg(start, stop, title, desc, lang, category)
}

/// Rust equivalent for `net_tcp_read` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buffer` should not be null. it should be of size `length`.
/// [`ccxr_start_tcp_srv`] or `start_tcp_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_tcp_read(
    _socket: c_int,
    buffer: *mut c_void,
    length: usize,
) -> c_int {
    let buffer = std::slice::from_raw_parts_mut(buffer as *mut u8, length);
    let ans = net_tcp_read(buffer);
    match ans {
        Some(x) => x.try_into().unwrap(),
        None => -1,
    }
}

/// Rust equivalent for `net_udp_read` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buffer` should not be null. it should be of size `length`.
/// [`ccxr_start_udp_srv`] or `start_udp_srv` must have been called before this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_net_udp_read(
    _socket: c_int,
    buffer: *mut c_void,
    length: usize,
    _src_str: *const c_char,
    _addr_str: *const c_char,
) -> c_int {
    let buffer = std::slice::from_raw_parts_mut(buffer as *mut u8, length);
    let ans = net_udp_read(buffer);
    match ans {
        Some(x) => x.try_into().unwrap(),
        None => -1,
    }
}

/// Rust equivalent for `start_tcp_srv` function in C. Uses C-native types as input and output.
///
/// Note that this function always returns 1 as an fd, since it will not be used anyway.
///
/// # Safety
///
/// `port` should be a numerical 16-bit value. All the strings must end with a nul character.
/// The output file desciptor should not be used.
#[no_mangle]
pub unsafe extern "C" fn ccxr_start_tcp_srv(port: *const c_char, pwd: *const c_char) -> c_int {
    let port = if !port.is_null() {
        Some(CStr::from_ptr(port).to_str().unwrap().parse().unwrap())
    } else {
        None
    };

    let pwd = if !pwd.is_null() {
        Some(CStr::from_ptr(pwd).to_str().unwrap())
    } else {
        None
    };

    start_tcp_srv(port, pwd);

    1
}

/// Rust equivalent for `start_udp_srv` function in C. Uses C-native types as input and output.
///
/// Note that this function always returns 1 as an fd, since it will not be used anyway.
///
/// # Safety
///
/// `port` should be a 16-bit value. All the strings must end with a nul character.
/// The output file desciptor should not be used.
#[no_mangle]
pub unsafe extern "C" fn ccxr_start_udp_srv(
    src: *const c_char,
    addr: *const c_char,
    port: c_uint,
) -> c_int {
    let src = if !src.is_null() {
        Some(CStr::from_ptr(src).to_str().unwrap())
    } else {
        None
    };

    let addr = if !addr.is_null() {
        Some(CStr::from_ptr(addr).to_str().unwrap())
    } else {
        None
    };

    let port = port.try_into().unwrap();

    start_udp_srv(src, addr, port);

    1
}
