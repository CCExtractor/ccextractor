use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};
use std::path::Path;
use std::{ptr, fs};

use crate::bindings::{cc_bitmap, cc_subtitle, encoder_ctx};

use crate::bindings::ccx_s_options;

use tesseract_sys::{TessBaseAPIInit4, TessBaseAPISetImage2};

#[derive(Debug)]
struct OcrCtx {
    api: *mut tesseract_sys::TessBaseAPI,
}

// global variables
extern "C" {
    static ccx_options: ccx_s_options;
    static language: *mut *mut ::std::os::raw::c_char;
}

#[no_mangle]
pub unsafe extern "C" fn init_ocr(
    lang_index: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_void {
    println!("ocr should be initialized here");
    let lang = CStr::from_ptr(ccx_options.ocrlang);
    dbg!(lang);
    let tessdata_path = probe_tessdata_location(lang);
    if tessdata_path.is_none() {
        println!("{}.traineddata not found! No switching possible", lang.to_string_lossy());
    }
    let tessdata_path = dbg!(tessdata_path.unwrap());
    let pars_vec_ptr = CString::new("debug_file").unwrap().into_raw();
    let pars_values_ptr = CString::new("tess.log").unwrap().into_raw();
    let tess_path_ptr = CString::new(format!("{tessdata_path}/tessdata")).unwrap().into_raw();


    let mut ctx = OcrCtx {
        api: tesseract_sys::TessBaseAPICreate(),
    };

    
    dbg!(TessBaseAPIInit4(ctx.api,
                    tess_path_ptr,
                    lang.as_ptr(), ccx_options.ocr_oem as u32, ptr::null_mut(), 0,
                    vec![pars_vec_ptr].as_mut_ptr(), vec![pars_values_ptr].as_mut_ptr(), 1, 0));

    println!("ocr initialized");

    drop(CString::from_raw(pars_vec_ptr));
    drop(CString::from_raw(pars_values_ptr));
    drop(CString::from_raw(tess_path_ptr));
    println!("memory freed");
    let ctx_ptr = &mut ctx as *mut _ as *mut c_void;
    std::mem::forget(ctx);
    ctx_ptr
}

fn probe_tessdata_location(lang: &CStr) -> Option<String> {
    let mut paths : Vec<String> = Vec::new();
    if let Ok(env_path) = std::env::var("TESSDATA_PREFIX") {
        paths.push(env_path);
    }
    paths.extend_from_slice(&[
        "./".to_string(),
	    "/usr/share/".to_string(),
	    "/usr/local/share/".to_string(),
	    "/usr/share/tesseract-ocr/".to_string(),
	    "/usr/share/tesseract-ocr/4.00/".to_string(),
	    "/usr/share/tesseract-ocr/5/".to_string(),
	    "/usr/share/tesseract/".to_string()
    ]);

    for path in paths {
        if search_language_pack(&path, lang) {
            return Some(path);
        }
    }

    None
}


fn search_language_pack(dir_name: &str, lang_name: &CStr) -> bool {
    if dir_name.is_empty() {
        return false;
    }

    let dir_path = Path::new(dir_name).join("tessdata");
    let desired_file_path = dir_path.clone().join(format!("{}.traineddata", lang_name.to_string_lossy()));
    let files = fs::read_dir(dir_path);
    if files.is_err() { return false;}
    let mut files = files.expect("tessdata location couldn't be accessed");
    return files.any(|file| -> bool {file.unwrap().path() == desired_file_path});
    

}

#[no_mangle]
pub unsafe extern "C" fn delete_ocr(arg: *mut *mut ::std::os::raw::c_void) {
    println!("here, ocr context is deleted");
}

#[no_mangle]
pub unsafe extern "C" fn ocr_rect(
    arg: *mut ::std::os::raw::c_void,
    rect: *mut cc_bitmap,
    str_: *mut *mut ::std::os::raw::c_char,
    bgcolor: ::std::os::raw::c_int,
    ocr_quantmode: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    println!("ocr rect called");
    dbg!(*rect);
    1
}

#[no_mangle]
pub unsafe extern "C" fn paraof_ocrtext(
    sub: *mut cc_subtitle,
    context: *mut encoder_ctx,
) -> *mut ::std::os::raw::c_char {
    let mut c = 'c';
    &mut c as *mut _ as *mut c_char
}
