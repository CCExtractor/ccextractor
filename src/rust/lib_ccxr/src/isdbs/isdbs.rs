extern crate libc;
use crate::isdbs::*;
use isdbd::*;

use std::alloc::{alloc, realloc, Layout};

use std::io::{self, Write};
use std::os::raw::{c_char, c_void};
use std::ptr;

#[allow(unused_assignments)]
#[allow(dead_code)]
#[allow(unused_comparisons)]
#[no_mangle]
pub fn is_horizontal_layout(format: WritingFormat) -> bool {
    matches!(
        format,
        WritingFormat::HorizontalStdDensity
            | WritingFormat::HorizontalHighDensity
            | WritingFormat::HorizontalWesternLang
            | WritingFormat::Horizontal1920x1080
            | WritingFormat::Horizontal960x540
            | WritingFormat::Horizontal720x480
            | WritingFormat::Horizontal1280x720
            | WritingFormat::HorizontalCustom
    )
}

#[no_mangle]
pub fn layout_get_width(format: WritingFormat) -> u32 {
    match format {
        WritingFormat::Horizontal960x540 | WritingFormat::Vertical960x540 => 960,
        _ => 720,
    }
}

#[no_mangle]
pub fn layout_get_height(format: WritingFormat) -> u32 {
    match format {
        WritingFormat::Horizontal960x540 | WritingFormat::Vertical960x540 => 540,
        _ => 480,
    }
}

pub fn isdb_set_global_time(ctx: &mut ISDBSubContext, timestamp: u64) {
    ctx.timestamp = timestamp;
}

pub fn init_layout(ls: *mut ISDBSubLayout) {
    unsafe {
        (*ls).font_size = 36;
        (*ls).display_area.x = 0;
        (*ls).display_area.y = 0;
        (*ls).font_scale.fscx = 100;
        (*ls).font_scale.fscy = 100;
    }
}

pub fn init_list_head(ptr: *mut ListHead) {
    unsafe {
        (*ptr).next = ptr;
        (*ptr).prev = ptr;
    }
}

extern "C" {
    pub fn free(ptr: *mut c_void);
}

pub unsafe fn freep(arg: *mut *mut c_void) {
    if !arg.is_null() && !(*arg).is_null() {
        free(*arg);
        *arg = ptr::null_mut();
    }
}

pub const LIST_POISON1: *mut ListHead = 0x001 as *mut ListHead;
pub const LIST_POISON2: *mut ListHead = 0x002 as *mut ListHead;

#[no_mangle]
pub unsafe fn __list_del(prev: *mut ListHead, next: *mut ListHead) {
    (*next).prev = prev;
    (*prev).next = next;
}

#[no_mangle]
pub unsafe fn list_del(entry: *mut ListHead) {
    __list_del((*entry).prev, (*entry).next);
    (*entry).next = LIST_POISON1;
    (*entry).prev = LIST_POISON2;
}

pub unsafe fn list_for_each_entry_safe(
    mut pos: *mut ISDBText,
    mut n: *mut ISDBText,
    head: *mut ListHead,
    member: *const u8,
) {
    pos = list_entry((*head).next, member);
    n = list_entry((*pos).list.next, member);
    while &(*pos).list as *const ListHead != head {
        // Your code to process each entry goes here
        pos = n;
        n = list_entry((*n).list.next, member);
    }
}

pub unsafe fn list_entry(ptr: *mut ListHead, member: *const u8) -> *mut ISDBText {
    container_of(ptr, member)
}

pub unsafe fn container_of(ptr: *mut ListHead, member: *const u8) -> *mut ISDBText {
    (ptr as *mut u8).offset(-(ccx_offsetof(member) as isize)) as *mut ISDBText
}

pub unsafe fn ccx_offsetof(_member: *const u8) -> usize {
    let offset = std::mem::offset_of!(ISDBText, list);
    offset
}

pub fn delete_isdb_decoder(ctx: *mut ISDBSubContext) {
    unsafe {
        let mut text: *mut ISDBText = ptr::null_mut();
        let mut text1: *mut ISDBText = ptr::null_mut();

        list_for_each_entry_safe(text, text1, &mut (*ctx).text_list_head, b"list\0".as_ptr());
        while !text.is_null() {
            list_del(&mut (*text).list);
            free((*text).buf as *mut c_void);
            free(text as *mut c_void);
            list_for_each_entry_safe(text, text1, &mut (*ctx).text_list_head, b"list\0".as_ptr());
        }

        list_for_each_entry_safe(text, text1, &mut (*ctx).buffered_text, b"list\0".as_ptr());
        while !text.is_null() {
            list_del(&mut (*text).list);
            free((*text).buf as *mut c_void);
            free(text as *mut c_void);
            list_for_each_entry_safe(text, text1, &mut (*ctx).buffered_text, b"list\0".as_ptr());
        }
    }
}

// this is in case DEBUG is not defined,
// which is the case in the C code
#[no_mangle]
pub fn isdb_log(args: std::fmt::Arguments) {
    // nothing said, nothing done
}

#[no_mangle]
pub fn allocate_text_node(_ls: &ISDBSubLayout) -> *mut ISDBText {
    unsafe {
        let text_layout = Layout::new::<ISDBText>();
        let text_ptr = alloc(text_layout) as *mut ISDBText;
        if text_ptr.is_null() {
            return ptr::null_mut();
        }

        (*text_ptr).used = 0;
        let buf_layout = Layout::from_size_align(128, 1).unwrap();
        let buf_ptr = alloc(buf_layout) as *mut u8;
        if buf_ptr.is_null() {
            return ptr::null_mut();
        }

        (*text_ptr).buf = buf_ptr as *mut i8;
        (*text_ptr).len = 128;
        *(*text_ptr).buf = 0;
        text_ptr
    }
}

pub fn reserve_buf(text: &mut ISDBText, len: usize) -> i32 {
    const CCX_OK: i32 = 0;
    const CCX_ENOMEM: i32 = 1;

    if text.len >= text.used + len {
        return CCX_OK;
    }

    // alloc always in 128 ka multiple
    let blen = ((text.used + len + 127) >> 7) << 7;
    unsafe {
        let layout = Layout::from_size_align(text.len, 1).unwrap();
        let new_ptr = realloc(text.buf as *mut u8, layout, blen) as *mut u8;
        if new_ptr.is_null() {
            isdb_log(format_args!("ISDB: out of memory for ctx->text.buf\n"));
            return CCX_ENOMEM;
        }
        text.buf = new_ptr as *mut i8;
    }
    text.len = blen;
    isdb_log(format_args!("expanded ctx->text({})\n", blen));
    CCX_OK
}

#[no_mangle]
pub unsafe fn __list_add(elem: *mut ListHead, prev: *mut ListHead, next: *mut ListHead) {
    (*next).prev = elem;
    (*elem).next = next;
    (*elem).prev = prev;
    (*prev).next = elem;
}

#[no_mangle]
pub unsafe fn list_add_tail(elem: *mut ListHead, head: *mut ListHead) {
    __list_add(elem, (*head).prev, head);
}

pub fn list_for_each_entry<F>(head: *mut ListHead, member: usize, mut func: F)
where
    F: FnMut(*mut ISDBText),
{
    unsafe {
        let member_ptr = member.to_string().as_ptr();
        let mut pos = list_entry((*head).next, member_ptr);
        while &(*pos).list as *const _ != head {
            func(pos);
            pos = list_entry((*pos).list.next, member_ptr);
        }
    }
}

pub fn list_add(elem: &mut ListHead, head: &mut ListHead) {
    unsafe {
        __list_add(elem, head, (*head).next);
    }
}

pub fn append_char(ctx: &mut ISDBSubContext, ch: char) -> i32 {
    let ls = &mut ctx.current_state.layout_state;
    let mut text: *mut ISDBText = std::ptr::null_mut();
    let cur_lpos;
    let csp;

    if is_horizontal_layout(ls.format) {
        cur_lpos = ls.cursor_pos.x;
        csp = ls.font_size * ls.font_scale.fscx / 100;
    } else {
        cur_lpos = ls.cursor_pos.y;
        csp = ls.font_size * ls.font_scale.fscy / 100;
    }

    unsafe {
        list_for_each_entry(
            &mut ctx.text_list_head as *mut ListHead,
            std::mem::offset_of!(ISDBText, list),
            |t| {
                text = t;
                let text_lpos = if is_horizontal_layout(ls.format) {
                    (*text).pos.x
                } else {
                    (*text).pos.y
                };

                if text_lpos == cur_lpos {
                    return;
                } else if text_lpos > cur_lpos {
                    let text1 = allocate_text_node(ls);
                    (*text1).pos.x = ls.cursor_pos.x;
                    (*text1).pos.y = ls.cursor_pos.y;
                    list_add(&mut (*text1).list, &mut *(*text).list.prev);
                    text = text1;
                    return;
                }
            },
        );

        if &(*text).list as *const ListHead == &ctx.text_list_head as *const ListHead {
            text = allocate_text_node(ls);
            (*text).pos.x = ls.cursor_pos.x;
            (*text).pos.y = ls.cursor_pos.y;
            list_add_tail(&mut (*text).list, &mut ctx.text_list_head);
        }

        if is_horizontal_layout(ls.format) {
            if ls.cursor_pos.y < (*text).pos.y {
                (*text).pos.y = ls.cursor_pos.y;
                (*text).used = 0;
            }
            ls.cursor_pos.y += csp;
            (*text).pos.y += csp;
        } else {
            if ls.cursor_pos.x < (*text).pos.x {
                (*text).pos.x = ls.cursor_pos.x;
                (*text).used = 0;
            }
            ls.cursor_pos.x += csp;
            (*text).pos.x += csp;
        }

        reserve_buf(&mut *text, 2);
        (*text).buf.add((*text).used).write(ch as u8 as i8);
        (*text).used += 1;
        (*text).buf.add((*text).used).write(0);

        1
    }
}

pub fn ccx_strstr_ignorespace(str1: *const c_char, str2: *const c_char) -> i32 {
    unsafe {
        let mut i = 0;
        while *str2.add(i) != 0 {
            if (*str2.add(i) as u8).is_ascii_whitespace() {
                i += 1;
                continue;
            }
            if *str2.add(i) != *str1.add(i) {
                return 0;
            }
            i += 1;
        }
        1
    }
}

pub fn list_empty(head: &ListHead) -> bool {
    head.next as *mut _ == head as *const _ as *mut _
}

pub fn get_text(ctx: &mut ISDBSubContext, buffer: &mut [u8], len: usize) -> usize {
    unsafe {
        let ls = &ctx.current_state.layout_state;
        let mut text: *mut ISDBText = std::ptr::null_mut();
        let mut sb_text: *mut ISDBText = std::ptr::null_mut();
        let mut sb_temp: *mut ISDBText = std::ptr::null_mut();

        // check no overflow for user
        let mut index = 0;

        if ctx.cfg_no_rollup != 0 || (ctx.cfg_no_rollup == ctx.current_state.rollup_mode) {
            if list_empty(&ctx.buffered_text) {
                list_for_each_entry(
                    &mut ctx.text_list_head as *mut ListHead,
                    std::mem::offset_of!(ISDBText, list),
                    |text| {
                        sb_text = allocate_text_node(ls);
                        list_add_tail(&mut (*sb_text).list, &mut ctx.buffered_text);
                        reserve_buf(&mut *sb_text, (*text).used);
                        std::ptr::copy_nonoverlapping((*text).buf, (*sb_text).buf, (*text).used);
                        *(*sb_text).buf.offset((*text).used as isize) = 0;
                        (*sb_text).used = (*text).used;
                    },
                );
                return 0;
            }

            list_for_each_entry(
                &mut ctx.text_list_head as *mut ListHead,
                std::mem::offset_of!(ISDBText, list),
                |text| {
                    let mut found = false;
                    list_for_each_entry(
                        &mut ctx.buffered_text as *mut ListHead,
                        std::mem::offset_of!(ISDBText, list),
                        |sb_text| {
                            if !found && ccx_strstr_ignorespace((*text).buf, (*sb_text).buf) != 0 {
                                found = true;
                                if ccx_strstr_ignorespace((*sb_text).buf, (*text).buf) == 0 {
                                    reserve_buf(&mut *sb_text, (*text).used);
                                    std::ptr::copy_nonoverlapping(
                                        (*text).buf,
                                        (*sb_text).buf,
                                        (*text).used,
                                    );
                                }
                            }
                        },
                    );
                    if !found {
                        sb_text = allocate_text_node(ls);
                        list_add_tail(&mut (*sb_text).list, &mut ctx.buffered_text);
                        reserve_buf(&mut *sb_text, (*text).used);
                        std::ptr::copy_nonoverlapping((*text).buf, (*sb_text).buf, (*text).used);
                        *(*sb_text).buf.offset((*text).used as isize) = 0;
                        (*sb_text).used = (*text).used;
                    }
                },
            );

            list_for_each_entry_safe(sb_text, sb_temp, &mut ctx.buffered_text, b"list\0".as_ptr());
            while !sb_text.is_null() {
                let mut found = false;
                list_for_each_entry(
                    &mut ctx.text_list_head as *mut ListHead,
                    std::mem::offset_of!(ISDBText, list),
                    |text| {
                        if ccx_strstr_ignorespace((*text).buf, (*sb_text).buf) != 0 {
                            found = true;
                        }
                    },
                );
                if !found && len - index > (*sb_text).used + 2 && (*sb_text).used > 0 {
                    std::ptr::copy_nonoverlapping(
                        (*sb_text).buf,
                        buffer[index..].as_mut_ptr() as *mut i8,
                        (*sb_text).used,
                    );
                    buffer[(*sb_text).used + index] = b'\n';
                    buffer[(*sb_text).used + index + 1] = b'\r';
                    index += (*sb_text).used + 2;
                    (*sb_text).used = 0;
                    list_del(&mut (*sb_text).list);
                    free((*sb_text).buf as *mut c_void);
                    free(sb_text as *mut c_void);
                }
                sb_text = sb_temp;
                sb_temp = list_entry((*sb_temp).list.next, b"list\0".as_ptr());
            }
        } else {
            list_for_each_entry(
                &mut ctx.text_list_head as *mut ListHead,
                std::mem::offset_of!(ISDBText, list),
                |text| {
                    if len - index > (*text).used + 2 && (*text).used > 0 {
                        std::ptr::copy_nonoverlapping(
                            (*text).buf,
                            buffer[index..].as_mut_ptr() as *mut i8,
                            (*text).used,
                        );
                        buffer[(*text).used + index] = b'\n';
                        buffer[(*text).used + index + 1] = b'\r';
                        index += (*text).used + 2;
                        (*text).used = 0;
                        *(*text).buf = 0;
                    }
                },
            );
        }
        index
    }
}

pub fn set_writing_format(ctx: &mut ISDBSubContext, mut arg: &[u8]) {
    let ls = &mut ctx.current_state.layout_state;

    if arg[1] == 0x20 {
        ls.format = match arg[0] & 0x0F {
            0 => WritingFormat::HorizontalStdDensity,
            1 => WritingFormat::VerticalStdDensity,
            2 => WritingFormat::HorizontalHighDensity,
            3 => WritingFormat::VerticalHighDensity,
            4 => WritingFormat::HorizontalWesternLang,
            5 => WritingFormat::Horizontal1920x1080,
            6 => WritingFormat::Vertical1920x1080,
            7 => WritingFormat::Horizontal960x540,
            8 => WritingFormat::Vertical960x540,
            9 => WritingFormat::Horizontal720x480,
            10 => WritingFormat::Vertical720x480,
            11 => WritingFormat::Horizontal1280x720,
            12 => WritingFormat::Vertical1280x720,
            _ => WritingFormat::None,
        };
        return;
    }

    if arg[1] == 0x3B {
        ls.format = WritingFormat::HorizontalCustom;
        arg = &arg[2..];
    }
    if arg[1] == 0x3B {
        match arg[0] & 0x0F {
            0 => {
                // ctx.font_size = FontSize::SmallFontSize;
            }
            1 => {
                // ctx.font_size = FontSize::MiddleFontSize;
            }
            2 => {
                // ctx.font_size = FontSize::StandardFontSize;
            }
            _ => {}
        }
        arg = &arg[2..];
    }

    isdb_log(format_args!("character numbers in one line in decimal:\0"));
    while arg[0] != 0x3B && arg[0] != 0x20 {
        ctx.nb_char = arg[0] as i32;
        println!(" {}", arg[0] & 0x0F);
        arg = &arg[1..];
    }
    if arg[0] == 0x20 {
        return;
    }
    arg = &arg[1..];
    isdb_log(format_args!("line numbers in decimal: "));
    while arg[0] != 0x20 {
        ctx.nb_line = arg[0] as i32;
        println!(" {}", arg[0] & 0x0F);
        arg = &arg[1..];
    }
}

pub fn move_penpos(ctx: &mut ISDBSubContext, col: i32, row: i32) {
    let ls = &mut ctx.current_state.layout_state;

    ls.cursor_pos.x = row;
    ls.cursor_pos.y = col;
}

pub fn set_position(ctx: &mut ISDBSubContext, p1: u32, p2: u32) {
    let ls = &mut ctx.current_state.layout_state;
    let (cw, ch, col, row);

    if is_horizontal_layout(ls.format) {
        cw = (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscx / 100;
        ch = (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscy / 100;
        // pen pos is bottom left
        col = p2 as i32 * cw;
        row = p1 as i32 * ch + ch;
    } else {
        cw = (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscy / 100;
        ch = (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscx / 100;
        // pen pos is upper center, -90deg rotated, hence mid left
        col = p2 as i32 * cw;
        row = p1 as i32 * ch + ch / 2;
    }
    move_penpos(ctx, col, row);
}

pub fn get_csi_params(q: &[u8], p1: &mut Option<u32>, p2: &mut Option<u32>) -> Result<i32, i32> {
    let mut q_pivot = 0;
    if p1.is_none() {
        return Err(-1);
    }

    *p1 = Some(0);
    while q[q_pivot] >= 0x30 && q[q_pivot] <= 0x39 {
        *p1.as_mut().unwrap() *= 10;
        *p1.as_mut().unwrap() += (q[q_pivot] - 0x30) as u32;
        q_pivot += 1;
    }
    if q[q_pivot] != 0x20 && q[q_pivot] != 0x3B {
        return Err(-1);
    }
    q_pivot += 1;
    if p2.is_none() {
        return Ok(q_pivot as i32);
    }
    *p2 = Some(0);
    while q[q_pivot] >= 0x30 && q[q_pivot] <= 0x39 {
        *p2.as_mut().unwrap() *= 10;
        *p2.as_mut().unwrap() += (q[q_pivot] - 0x30) as u32;
        q_pivot += 1;
    }
    q_pivot += 1;
    Ok(q_pivot as i32)
}

pub fn isdb_command_log(_format: &str, _args: std::fmt::Arguments) {
    // nothing said, nothing done
}

pub fn parse_csi(ctx: &mut ISDBSubContext, buf: &[u8]) -> i32 {
    let mut arg = [0u8; 10];
    let mut i = 0;
    let mut ret = 0;
    let mut p1 = None;
    let mut p2 = None;
    let buf_pivot = buf;
    let state = &mut ctx.current_state;
    let ls = &mut state.layout_state;

    // cpy buf in arg
    while buf[i] != 0x20 {
        if i >= arg.len() {
            isdb_log(format_args!("UnExpected CSI {} >= {}\n", arg.len(), i));
            break;
        }
        arg[i] = buf[i];
        i += 1;
    }
    // ignore terminating 0x20 char
    arg[i] = buf[i];
    i += 1;

    match buf[i] {
        // Writing Format setting
        CSI_CMD_SWF => {
            isdb_command_log("Command:CSI: SWF\n", format_args!(""));
            set_writing_format(ctx, &arg);
        }
        // Composite Character compose
        CSI_CMD_CCC => {
            isdb_command_log("Command:CSI: CCC\n", format_args!(""));
            ret = get_csi_params(&arg, &mut p1, &mut None).unwrap_or(-1);
            if ret > 0 {
                ls.ccc = match p1.unwrap() {
                    0 => IsdbCCComposition::None,
                    2 => IsdbCCComposition::And,
                    3 => IsdbCCComposition::Or,
                    4 => IsdbCCComposition::Xor,
                    _ => IsdbCCComposition::None,
                };
            }
        }
        // Display Format setting
        CSI_CMD_SDF => {
            ret = get_csi_params(&arg, &mut p1, &mut p2).unwrap_or(-1);
            if ret > 0 {
                ls.display_area.w = p1.unwrap() as i32;
                ls.display_area.h = p2.unwrap() as i32;
            }
            isdb_command_log(
                "Command:CSI: SDF (w:{}, h:{})\n",
                format_args!("{}, {}", p1.unwrap(), p2.unwrap()),
            );
        }
        // char composition's dot designation
        CSI_CMD_SSM => {
            ret = get_csi_params(&arg, &mut p1, &mut p2).unwrap_or(-1);
            if ret > 0 {
                ls.font_size = p1.unwrap() as i32;
            }
            isdb_command_log(
                "Command:CSI: SSM (x:{}, y:{})\n",
                format_args!("{}, {}", p1.unwrap(), p2.unwrap()),
            );
        }
        // Set Display Position
        CSI_CMD_SDP => {
            ret = get_csi_params(&arg, &mut p1, &mut p2).unwrap_or(-1);
            if ret > 0 {
                ls.display_area.x = p1.unwrap() as i32;
                ls.display_area.y = p2.unwrap() as i32;
            }
            isdb_command_log(
                "Command:CSI: SDP (x:{}, y:{})\n",
                format_args!("{}, {}", p1.unwrap(), p2.unwrap()),
            );
        }
        // Raster Colour command
        CSI_CMD_RCS => {
            ret = get_csi_params(&arg, &mut p1, &mut None).unwrap_or(-1);
            if ret > 0 {
                ctx.current_state.raster_color = DEFAULT_CLUT
                    [(ctx.current_state.clut_high_idx << 4 | p1.unwrap() as u8) as usize];
            }
            isdb_command_log("Command:CSI: RCS ({})\n", format_args!("{}", p1.unwrap()));
        }
        // Set Horizontal Spacing
        CSI_CMD_SHS => {
            ret = get_csi_params(&arg, &mut p1, &mut None).unwrap_or(-1);
            if ret > 0 {
                ls.cell_spacing.col = p1.unwrap() as i32;
            }
            isdb_command_log("Command:CSI: SHS ({})\n", format_args!("{}", p1.unwrap()));
        }
        // Set Vertical Spacing
        CSI_CMD_SVS => {
            ret = get_csi_params(&arg, &mut p1, &mut None).unwrap_or(-1);
            if ret > 0 {
                ls.cell_spacing.row = p1.unwrap() as i32;
            }
            isdb_command_log("Command:CSI: SVS ({})\n", format_args!("{}", p1.unwrap()));
        }
        // Active Coordinate Position Set
        CSI_CMD_ACPS => {
            isdb_command_log("Command:CSI: ACPS\n", format_args!(""));
            ret = get_csi_params(&arg, &mut p1, &mut p2).unwrap_or(-1);
            if ret > 0 {
                ls.acps[0] = p1.unwrap() as i32;
                ls.acps[1] = p2.unwrap() as i32;
            }
        }
        _ => {
            isdb_log(format_args!(
                "Command:CSI: Unknown command 0x{:x}\n",
                buf[i]
            ));
        }
    }
    i as i32
}

pub fn parse_command(ctx: &mut ISDBSubContext, buf: &[u8]) -> i32 {
    let buf_pivot = buf;
    let code_lo = buf[0] & 0x0f;
    let code_hi = (buf[0] & 0xf0) >> 4;
    let mut ret;
    let state = &mut ctx.current_state;
    let ls = &mut state.layout_state;

    let mut buf = &buf[1..];
    match code_hi {
        0x00 => match code_lo {
            0x0 => isdb_command_log("Command: NUL\n", format_args!("")),
            0x7 => isdb_command_log("Command: BEL\n", format_args!("")),
            0x8 => isdb_command_log("Command: ABP\n", format_args!("")),
            0x9 => isdb_command_log("Command: APF\n", format_args!("")),
            0xA => isdb_command_log("Command: APD\n", format_args!("")),
            0xB => isdb_command_log("Command: APU\n", format_args!("")),
            0xC => isdb_command_log("Command: CS clear Screen\n", format_args!("")),
            0xD => isdb_command_log("Command: APR\n", format_args!("")),
            0xE => isdb_command_log("Command: LS1\n", format_args!("")),
            0xF => isdb_command_log("Command: LS0\n", format_args!("")),
            _ => isdb_command_log("Command: Unknown\n", format_args!("")),
        },
        0x01 => match code_lo {
            0x6 => isdb_command_log("Command: PAPF\n", format_args!("")),
            0x8 => isdb_command_log("Command: CAN\n", format_args!("")),
            0x9 => isdb_command_log("Command: SS2\n", format_args!("")),
            0xB => isdb_command_log("Command: ESC\n", format_args!("")),
            0xC => {
                isdb_command_log("Command: APS\n", format_args!(""));
                set_position(ctx, (buf[0] & 0x3F) as u32, (buf[1] & 0x3F) as u32);
                buf = &buf[2..];
            }
            0xD => isdb_command_log("Command: SS3\n", format_args!("")),
            0xE => isdb_command_log("Command: RS\n", format_args!("")),
            0xF => isdb_command_log("Command: US\n", format_args!("")),
            _ => isdb_command_log("Command: Unknown\n", format_args!("")),
        },
        0x8 => match code_lo {
            0x0..=0x7 => {
                isdb_command_log(
                    &format!(
                        "Command: Forground color (0x{:X})\n",
                        DEFAULT_CLUT[code_lo as usize]
                    ),
                    format_args!(""),
                );
                state.fg_color = DEFAULT_CLUT[code_lo as usize];
            }
            0x8 => {
                isdb_command_log("Command: SSZ\n", format_args!(""));
                ls.font_scale.fscx = 50;
                ls.font_scale.fscy = 50;
            }
            0x9 => {
                isdb_command_log("Command: MSZ\n", format_args!(""));
                ls.font_scale.fscx = 200;
                ls.font_scale.fscy = 200;
            }
            0xA => {
                isdb_command_log("Command: NSZ\n", format_args!(""));
                ls.font_scale.fscx = 100;
                ls.font_scale.fscy = 100;
            }
            0xB => {
                isdb_command_log("Command: SZX\n", format_args!(""));
                buf = &buf[1..];
            }
            _ => isdb_command_log("Command: Unknown\n", format_args!("")),
        },
        0x9 => match code_lo {
            0x0 => {
                if buf[0] == 0x20 {
                    isdb_command_log(
                        &format!("Command: COL: Set Clut {}\n", (buf[0] & 0x0F)),
                        format_args!(""),
                    );
                    buf = &buf[1..];
                    ctx.current_state.clut_high_idx = (buf[0] & 0x0F) as u8;
                } else if (buf[0] & 0xF0) == 0x40 {
                    isdb_command_log(
                        &format!(
                            "Command: COL: Set Forground 0x{:08X}\n",
                            DEFAULT_CLUT[(buf[0] & 0x0F) as usize]
                        ),
                        format_args!(""),
                    );
                    ctx.current_state.fg_color = DEFAULT_CLUT[(buf[0] & 0x0F) as usize];
                } else if (buf[0] & 0xF0) == 0x50 {
                    isdb_command_log(
                        &format!(
                            "Command: COL: Set Background 0x{:08X}\n",
                            DEFAULT_CLUT[(buf[0] & 0x0F) as usize]
                        ),
                        format_args!(""),
                    );
                    ctx.current_state.bg_color = DEFAULT_CLUT[(buf[0] & 0x0F) as usize];
                } else if (buf[0] & 0xF0) == 0x60 {
                    isdb_command_log(
                        &format!(
                            "Command: COL: Set half Forground 0x{:08X}\n",
                            DEFAULT_CLUT[(buf[0] & 0x0F) as usize]
                        ),
                        format_args!(""),
                    );
                    ctx.current_state.hfg_color = DEFAULT_CLUT[(buf[0] & 0x0F) as usize];
                } else if (buf[0] & 0xF0) == 0x70 {
                    isdb_command_log(
                        &format!(
                            "Command: COL: Set Half Background 0x{:08X}\n",
                            DEFAULT_CLUT[(buf[0] & 0x0F) as usize]
                        ),
                        format_args!(""),
                    );
                    ctx.current_state.hbg_color = DEFAULT_CLUT[(buf[0] & 0x0F) as usize];
                }
                buf = &buf[1..];
            }
            0x1 => {
                isdb_command_log("Command: FLC\n", format_args!(""));
                buf = &buf[1..];
            }
            0x2 => {
                isdb_command_log("Command: CDC\n", format_args!(""));
                buf = &buf[3..];
            }
            0x3 => {
                isdb_command_log("Command: POL\n", format_args!(""));
                buf = &buf[1..];
            }
            0x4 => {
                isdb_command_log("Command: WMM\n", format_args!(""));
                buf = &buf[3..];
            }
            0x5 => {
                isdb_command_log("Command: MACRO\n", format_args!(""));
                buf = &buf[1..];
            }
            0x7 => {
                isdb_command_log("Command: HLC\n", format_args!(""));
                buf = &buf[1..];
            }
            0x8 => {
                isdb_command_log("Command: RPC\n", format_args!(""));
                buf = &buf[1..];
            }
            0x9 => isdb_command_log("Command: SPL\n", format_args!("")),
            0xA => isdb_command_log("Command: STL\n", format_args!("")),
            0xB => {
                ret = parse_csi(ctx, buf);
                buf = &buf[ret as usize..];
            }
            0xD => {
                isdb_command_log("Command: TIME\n", format_args!(""));
                buf = &buf[2..];
            }
            _ => isdb_command_log("Command: Unknown\n", format_args!("")),
        },
        _ => isdb_command_log("Command: Unknown\n", format_args!("")),
    }

    (buf_pivot.len() - buf.len()) as i32
}

pub fn parse_caption_management_data(ctx: &mut ISDBSubContext, buf: &[u8]) -> i32 {
    let buf_pivot = buf;
    let mut buf = buf;

    ctx.tmd = match buf[0] >> 6 {
        0 => IsdbTmd::Free,
        1 => IsdbTmd::RealTime,
        2 => IsdbTmd::OffsetTime,
        _ => IsdbTmd::Free, // default case
    };
    isdb_log(format_args!("CC MGMT DATA: TMD: {:?}\n", ctx.tmd));
    buf = &buf[1..];

    match ctx.tmd {
        ISDB_TMD_FREE => {
            isdb_log(format_args!(
                "Playback time is not restricted to synchronize to the clock.\n"
            ));
        }
        ISDB_TMD_OFFSET_TIME => {
            ctx.offset_time.hour = ((buf[0] >> 4) * 10 + (buf[0] & 0xf)) as i32;
            buf = &buf[1..];
            ctx.offset_time.min = ((buf[0] >> 4) * 10 + (buf[0] & 0xf)) as i32;
            buf = &buf[1..];
            ctx.offset_time.sec = ((buf[0] >> 4) * 10 + (buf[0] & 0xf)) as i32;
            buf = &buf[1..];
            ctx.offset_time.milli =
                ((buf[0] >> 4) * 100 + ((buf[0] & 0xf) * 10) + (buf[1] & 0xf)) as i32;
            buf = &buf[2..];
            isdb_log(format_args!(
                "CC MGMT DATA: OTD( h:{} m:{} s:{} millis: {}\n",
                ctx.offset_time.hour,
                ctx.offset_time.min,
                ctx.offset_time.sec,
                ctx.offset_time.milli
            ));
        }
        _ => {
            isdb_log(format_args!(
                "Playback time is in accordance with the time of the clock, \
                which is calibrated by clock signal (TDT). Playback time is \
                given by PTS.\n"
            ));
        }
    }
    ctx.nb_lang = buf[0] as i32;
    isdb_log(format_args!(
        "CC MGMT DATA: nb languages: {}\n",
        ctx.nb_lang
    ));
    buf = &buf[1..];

    for _ in 0..ctx.nb_lang {
        isdb_log(format_args!("CC MGMT DATA: {}\n", (buf[0] & 0x1F) >> 5));
        ctx.dmf = buf[0] & 0x0F;
        isdb_log(format_args!("CC MGMT DATA: DMF 0x{:X}\n", ctx.dmf));
        buf = &buf[1..];
        if ctx.dmf == 0xC || ctx.dmf == 0xD || ctx.dmf == 0xE {
            ctx.dc = buf[0];
            if ctx.dc == 0x00 {
                isdb_log(format_args!("Attenuation Due to Rain\n"));
            }
        }
        isdb_log(format_args!(
            "CC MGMT DATA: languages: {}{}{}\n",
            buf[0] as char, buf[1] as char, buf[2] as char
        ));
        buf = &buf[3..];
        isdb_log(format_args!("CC MGMT DATA: Format: 0x{:X}\n", buf[0] >> 4));
        isdb_log(format_args!(
            "CC MGMT DATA: TCS: 0x{:X}\n",
            (buf[0] >> 2) & 0x3
        ));
        ctx.current_state.rollup_mode = if (buf[0] & 0x3) != 0 { 1 } else { 0 };
        isdb_log(format_args!(
            "CC MGMT DATA: Rollup mode: 0x{:X}\n",
            ctx.current_state.rollup_mode
        ));
        buf = &buf[1..];
    }
    (buf_pivot.len() - buf.len()) as i32
}

// in/from ccx_common_common.c
use std::ffi::CString;
pub fn add_cc_sub_text(
    sub: &mut CcSubtitle,
    str: &str,
    start_time: i64,
    end_time: i64,
    info: Option<&str>,
    mode: Option<&str>,
    e_type: CcxEncodingType,
) -> i32 {
    if str.is_empty() {
        return 0;
    }

    let target_sub = if sub.nb_data > 0 {
        let mut current = sub;
        while !current.next.is_null() {
            current = unsafe { &mut *current.next };
        }
        let new_sub = Box::into_raw(Box::new(CcSubtitle {
            data: ptr::null_mut(),
            datatype: Subdatatype::Generic,
            nb_data: 0,
            type_: Subtype::Text,
            enc_type: e_type,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: 0,
            mode: [0; 5],
            info: [0; 4],
            time_out: 0,
            next: ptr::null_mut(),
            prev: current,
        }));
        current.next = new_sub;
        unsafe { &mut *new_sub }
    } else {
        sub
    };

    target_sub.type_ = Subtype::Text;
    target_sub.enc_type = e_type;
    target_sub.data = CString::new(str).unwrap().into_raw() as *mut c_void;
    target_sub.datatype = Subdatatype::Generic;
    target_sub.nb_data = str.len() as u32;
    target_sub.start_time = start_time;
    target_sub.end_time = end_time;
    if let Some(info_str) = info {
        let info_bytes = info_str.as_bytes();
        target_sub.info[..info_bytes.len()].copy_from_slice(&info_bytes[..4]);
    }
    if let Some(mode_str) = mode {
        let mode_bytes = mode_str.as_bytes();
        target_sub.mode[..mode_bytes.len()].copy_from_slice(&mode_bytes[..4]);
    }
    target_sub.got_output = 1;
    target_sub.next = ptr::null_mut();

    0
}

pub fn parse_statement(ctx: &mut ISDBSubContext, buf: &[u8], size: i32) -> i32 {
    let buf_pivot = buf;
    let mut buf = buf;
    let mut ret;

    while (buf.len() - buf_pivot.len()) < size as usize {
        let code = (buf[0] & 0xf0) >> 4;
        let code_lo = buf[0] & 0x0f;
        if code <= 0x1 {
            ret = parse_command(ctx, buf);
        } else if code == 0x2 && code_lo == 0x0 {
            ret = append_char(ctx, buf[0] as char);
        } else if code == 0x7 && code_lo == 0xF {
            ret = append_char(ctx, buf[0] as char);
        } else if code <= 0x7 {
            ret = append_char(ctx, buf[0] as char);
        } else if code <= 0x9 {
            ret = parse_command(ctx, buf);
        } else if code == 0xA && code_lo == 0x0 {
            // still todo :)
            ret = 1;
        } else if code == 0x0F && code_lo == 0xF {
            // still todo :)
            ret = 1;
        } else {
            ret = append_char(ctx, buf[0] as char);
        }
        if ret < 0 {
            break;
        }
        buf = &buf[ret as usize..];
    }
    0
}

pub fn parse_data_unit(ctx: &mut ISDBSubContext, buf: &[u8], size: i32) -> i32 {
    let mut buf = buf;
    // unit separator
    buf = &buf[1..];

    let unit_parameter = buf[0];
    buf = &buf[1..];
    let len = rb24(buf);
    buf = &buf[3..];

    match unit_parameter {
        // statement body
        0x20 => {
            parse_statement(ctx, buf, len);
        }
        _ => {}
    }
    0
}

pub fn parse_caption_statement_data(
    ctx: &mut ISDBSubContext,
    lang_id: i32,
    buf: &[u8],
    size: i32,
    sub: &mut CcSubtitle,
) -> i32 {
    let mut buf = buf;
    let tmd = buf[0] >> 6;
    buf = &buf[1..];

    // skipping timing data
    if tmd == 1 || tmd == 2 {
        buf = &buf[5..];
    }

    let len = rb24(buf);
    buf = &buf[3..];
    let ret = parse_data_unit(ctx, buf, len);
    if ret < 0 {
        return -1;
    }

    let mut buffer = [0u8; 1024];
    let ret = get_text(ctx, &mut buffer, 1024);
    // cpy data if there in buffer
    if ret < 0 {
        return 0;
    }

    if ret > 0 {
        add_cc_sub_text(
            sub,
            std::str::from_utf8(&buffer).unwrap_or(""),
            ctx.prev_timestamp as i64,
            ctx.timestamp as i64,
            Some("NA"),
            Some("ISDB"),
            CcxEncodingType::Utf8,
        );
        if sub.start_time == sub.end_time {
            sub.end_time += 2;
        }
        ctx.prev_timestamp = ctx.timestamp;
    }
    0
}

fn rb24(buf: &[u8]) -> i32 {
    ((buf[0] as i32) << 16) | ((buf[1] as i32) << 8) | (buf[2] as i32)
}

pub static mut CCX_OPTIONS: CcxOptions = CcxOptions {
    messages_target: CCX_MESSAGES_QUIET,
};

pub fn mprint(message: &str) {
    unsafe {
        if CCX_OPTIONS.messages_target == CCX_MESSAGES_QUIET {
            return;
        }

        match CCX_OPTIONS.messages_target {
            CCX_MESSAGES_STDOUT => {
                print!("{}", message);
                io::stdout().flush().unwrap();
            }
            CCX_MESSAGES_STDERR => {
                eprint!("{}", message);
                io::stderr().flush().unwrap();
            }
            _ => (),
        }
    }
}

pub fn cstr(s: &str) -> std::ffi::CString {
    std::ffi::CString::new(s).unwrap()
}

pub fn isdb_parse_data_group(codec_ctx: *mut c_void, buf: &[u8], sub: &mut CcSubtitle) -> i32 {
    let ctx = unsafe { &mut *(codec_ctx as *mut ISDBSubContext) };
    let buf_pivot = buf;
    let mut buf = buf;
    let id = buf[0] >> 2;
    let mut group_size = 0;
    let mut ret = 0;

    if (id >> 4) == 0 {
        isdb_log(format_args!("ISDB group A\n"));
    } else if (id >> 4) == 0 {
        isdb_log(format_args!("ISDB group B\n"));
    }

    buf = &buf[3..];

    group_size = rb16(buf);
    buf = &buf[2..];
    isdb_log(format_args!(
        "ISDB (Data group) group_size {}\n",
        group_size
    ));

    if ctx.prev_timestamp > ctx.timestamp {
        ctx.prev_timestamp = ctx.timestamp;
    }
    if (id & 0x0F) == 0 {
        // for caption management
        ret = parse_caption_management_data(ctx, buf);
    } else if (id & 0x0F) < 8 {
        // caption sttmnt data
        isdb_log(format_args!("ISDB {} language\n", id));
        ret = parse_caption_statement_data(ctx, (id & 0x0F) as i32, buf, group_size as i32, sub);
    } else {
        // coz not allowed in spec
    }
    if ret < 0 {
        return -1;
    }
    buf = &buf[group_size as usize..];

    // crc todo still :)
    buf = &buf[2..];

    (buf_pivot.len() - buf.len()) as i32
}

fn rb16(buf: &[u8]) -> i32 {
    ((buf[0] as i32) << 8) | (buf[1] as i32)
}

pub fn isdbsub_decode(dec_ctx: &mut LibCcDecode, buf: &[u8], sub: &mut CcSubtitle) -> i32 {
    let mut buf = buf;
    let mut ret = 0;
    let ctx = unsafe { &mut *(dec_ctx.private_data as *mut ISDBSubContext) };

    if buf[0] != 0x80 {
        mprint("Not a Syncronised PES\n");
        return -1;
    }
    buf = &buf[1..];

    // pvt data stream is 0xFF
    buf = &buf[1..];
    let header_end = buf[0] & 0x0f;
    buf = &buf[1..];

    while buf.len() > header_end as usize {
        // todo header spec finding :)
        buf = &buf[1..];
    }

    ctx.cfg_no_rollup = dec_ctx.no_rollup;
    ret = isdb_parse_data_group(ctx as *mut _ as *mut c_void, buf, sub);
    if ret < 0 {
        return -1;
    }

    1
}
