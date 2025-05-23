use crate::decoder_isdb::exit_codes::*;
use crate::decoder_isdb::functions_common::*;
use crate::decoder_isdb::structs_ccdecode::*;
use crate::decoder_isdb::structs_isdb::*;
use crate::decoder_isdb::structs_xds::*;

use std::os::raw::c_char;

use log::info; // use info!() for isdb_log and isdb_command_log

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

/// # Safety
///
/// - The caller must ensure that `ls` is a valid, non-null pointer to an `ISDBSubLayout`.
/// - Improper usage of the pointer may result in undefined behavior.
pub unsafe fn init_layout(ls: *mut ISDBSubLayout) {
    unsafe {
        (*ls).font_size = 36;
        (*ls).display_area.x = 0;
        (*ls).display_area.y = 0;
        (*ls).font_scale.fscx = 100;
        (*ls).font_scale.fscy = 100;
    }
}

/// # Safety
///
/// - The caller must ensure that `text` is a valid, mutable reference to an `ISDBText`.
/// - The function manipulates raw pointers and performs unsafe operations, so improper usage may result in undefined behavior.
pub fn reserve_buf(text: &mut ISDBText, len: usize) -> Result<(), &'static str> {
    // Check if the current buffer length is sufficient
    if text.len >= text.used + len {
        return Ok(());
    }

    // Calculate the new buffer length (always in multiples of 128)
    let blen = ((text.used + len + 127) >> 7) << 7;

    // Attempt to resize the buffer
    let mut new_buf = Vec::with_capacity(blen);
    new_buf.extend_from_slice(unsafe { std::slice::from_raw_parts(text.buf, text.used) });

    // Update the buffer and its length
    text.buf = new_buf.as_mut_ptr();
    text.len = blen;

    // Log the expansion
    info!("expanded ctx->text({})", blen);

    Ok(())
}

/// # Safety
///
/// - The caller must ensure that `str1` and `str2` are valid, null-terminated C-style strings.
/// - Passing invalid or null pointers will result in undefined behavior.
pub unsafe fn ccx_strstr_ignorespace(str1: *const c_char, str2: *const c_char) -> i32 {
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

/// # Safety
///
/// - The caller must ensure that `isdb_ctx` is a valid, mutable reference to an `Option<Box<ISDBSubContext>>`.
/// - The function performs unsafe operations to free memory, so improper usage may result in undefined behavior.
pub fn delete_isdb_decoder(isdb_ctx: &mut Option<Box<ISDBSubContext>>) {
    if let Some(ctx) = isdb_ctx.as_mut() {
        // Iterate over `text_list_head` and free its elements
        while let Some(text) = unsafe { (ctx.text_list_head.next as *mut ISDBText).as_mut() } {
            unsafe {
                // Remove the element from the list
                (*text.list.prev).next = text.list.next;
                (*text.list.next).prev = text.list.prev;
            }
            // Free the buffer
            if !text.buf.is_null() {
                unsafe { drop(Box::from_raw(text.buf)) };
            }
            // Free the text structure
            unsafe { drop(Box::from_raw(text)) };
        }

        // Iterate over `buffered_text` and free its elements
        while let Some(text) = unsafe { (ctx.buffered_text.next as *mut ISDBText).as_mut() } {
            unsafe {
                // Remove the element from the list
                (*text.list.prev).next = text.list.next;
                (*text.list.next).prev = text.list.prev;
            }
            // Free the buffer
            if !text.buf.is_null() {
                unsafe { drop(Box::from_raw(text.buf)) };
            }
            // Free the text structure
            unsafe { drop(Box::from_raw(text)) };
        }

        // Free the ISDBSubContext itself
        *isdb_ctx = None;
    }
}

/// # Safety
///
/// - This function is safe as it does not perform any unsafe operations.
/// - The caller is responsible for managing the lifetime of the returned `Box<ISDBSubContext>`.
pub fn init_isdb_decoder() -> Option<Box<ISDBSubContext>> {
    // Allocate memory for ISDBSubContext
    let mut ctx = Box::new(ISDBSubContext {
        text_list_head: ListHead {
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        },
        buffered_text: ListHead {
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        },
        prev_timestamp: u64::MAX,
        current_state: ISDBSubState {
            clut_high_idx: 0,
            rollup_mode: 0,
            ..Default::default()
        },
        ..Default::default()
    });

    // Initialize the list heads
    ctx.text_list_head.next = &mut ctx.text_list_head as *mut ListHead;
    ctx.text_list_head.prev = &mut ctx.text_list_head as *mut ListHead;
    ctx.buffered_text.next = &mut ctx.buffered_text as *mut ListHead;
    ctx.buffered_text.prev = &mut ctx.buffered_text as *mut ListHead;

    // Initialize the layout state
    unsafe {
        init_layout(&mut ctx.current_state.layout_state);
    }

    Some(ctx)
}

/// # Safety
///
/// - The caller must ensure that `ls` is a valid, mutable reference to an `ISDBSubLayout`.
/// - The function manipulates raw pointers and performs unsafe operations, so improper usage may result in undefined behavior.
pub fn allocate_text_node(ls: &mut ISDBSubLayout) -> Option<Box<ISDBText>> {
    // assign ls to uls
    let _uls = ls as *mut ISDBSubLayout; // ls unused

    // Allocate memory for the ISDBText structure
    let mut text = Box::new(ISDBText {
        buf: std::ptr::null_mut(),
        len: 128,
        used: 0,
        pos: ISDBPos { x: 0, y: 0 },
        txt_tail: 0,
        timestamp: 0,
        refcount: 0,
        list: ListHead {
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        },
    });

    // Allocate memory for the buffer
    let mut buf = vec![0u8; 128];
    text.buf = buf.as_mut_ptr() as *mut c_char;
    std::mem::forget(buf); // Prevent Rust from deallocating the buffer

    Some(text)
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The function manipulates raw pointers and performs unsafe operations, so improper usage may result in undefined behavior.
pub fn append_char(ctx: &mut ISDBSubContext, ch: char) -> i32 {
    let ls = &mut ctx.current_state.layout_state;
    let mut text: Option<&mut ISDBText> = None;

    // Current Line Position
    let cur_lpos;
    // Space taken by character
    let csp;

    if is_horizontal_layout(ls.format) {
        cur_lpos = ls.cursor_pos.x;
        csp = ls.font_size * ls.font_scale.fscx / 100;
    } else {
        cur_lpos = ls.cursor_pos.y;
        csp = ls.font_size * ls.font_scale.fscy / 100;
    }

    // Iterate over the text list
    let mut current = ctx.text_list_head.next;
    while let Some(current_text) = unsafe { (current as *mut ISDBText).as_mut() } {
        // Text Line Position
        let text_lpos = if is_horizontal_layout(ls.format) {
            current_text.pos.x
        } else {
            current_text.pos.y
        };

        match text_lpos.cmp(&cur_lpos) {
            std::cmp::Ordering::Equal => {
                text = Some(current_text);
                break;
            }
            std::cmp::Ordering::Greater => {
                // Allocate Text here so that list is always sorted
                let mut new_text = allocate_text_node(ls).expect("Failed to allocate text node");
                new_text.pos.x = ls.cursor_pos.x;
                new_text.pos.y = ls.cursor_pos.y;

                // Add the new text node before the current one
                unsafe {
                    (*new_text.list.prev).next = &mut new_text.list as *mut ListHead;
                    (*new_text.list.next).prev = &mut new_text.list as *mut ListHead;
                }

                text = Some(&mut *Box::leak(new_text));
                break;
            }
            _ => {}
        }

        current = current_text.list.next;
    }

    if text.is_none() {
        // If no matching text node was found, allocate a new one and add it to the tail
        let mut new_text = allocate_text_node(ls).expect("Failed to allocate text node");
        new_text.pos.x = ls.cursor_pos.x;
        new_text.pos.y = ls.cursor_pos.y;

        // Add the new text node to the tail
        unsafe {
            (*ctx.text_list_head.prev).next = &mut new_text.list as *mut ListHead;
            (*new_text.list.prev).prev = &mut ctx.text_list_head as *mut ListHead;
        }

        text = Some(Box::leak(new_text));
    }

    let text = text.unwrap();

    // Check position of character and if moving backward then clean text
    if is_horizontal_layout(ls.format) {
        if ls.cursor_pos.y < text.pos.y {
            text.pos.y = ls.cursor_pos.y;
            text.used = 0;
        }
        ls.cursor_pos.y += csp;
        text.pos.y += csp;
    } else {
        if ls.cursor_pos.y < text.pos.y {
            text.pos.y = ls.cursor_pos.y;
            text.used = 0;
        }
        ls.cursor_pos.x += csp;
        text.pos.x += csp;
    }

    // Reserve buffer space for the character
    reserve_buf(text, 2).expect("Failed to reserve buffer space"); // +1 for terminating '\0'

    // Append the character to the buffer
    unsafe {
        *text.buf.add(text.used) = ch as i8;
        text.used += 1;
        *text.buf.add(text.used) = b'\0' as i8;
    }

    1
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buffer` must be a valid, mutable slice with sufficient capacity to hold the text.
/// - The function manipulates raw pointers and performs unsafe operations, so improper usage may result in undefined behavior.
pub fn get_text(ctx: &mut ISDBSubContext, buffer: &mut [u8], len: usize) -> usize {
    let ls = &mut ctx.current_state.layout_state;
    let mut index = 0;

    if ctx.cfg_no_rollup != 0 || ctx.cfg_no_rollup == ctx.current_state.rollup_mode {
        // If the buffered_text list is empty, copy text_list_head to buffered_text
        if ctx.buffered_text.next == &mut ctx.buffered_text as *mut ListHead {
            let mut current = ctx.text_list_head.next;
            while let Some(text) = unsafe { (current as *mut ISDBText).as_mut() } {
                let mut sb_text = allocate_text_node(ls).expect("Failed to allocate text node");
                unsafe {
                    // Add to buffered_text
                    sb_text.list.next = &mut ctx.buffered_text as *mut ListHead;
                    sb_text.list.prev = ctx.buffered_text.prev;
                    (*ctx.buffered_text.prev).next = &mut sb_text.list as *mut ListHead;
                    ctx.buffered_text.prev = &mut sb_text.list as *mut ListHead;
                }
                reserve_buf(&mut sb_text, text.used).expect("Failed to reserve buffer space");
                unsafe {
                    std::ptr::copy_nonoverlapping(text.buf, sb_text.buf, text.used);
                    *sb_text.buf.add(text.used) = b'\0' as i8;
                }
                sb_text.used = text.used;

                current = text.list.next;
            }
            return 0;
        }

        // Update buffered_text for new entries in text_list_head
        let mut current = ctx.text_list_head.next;
        while let Some(text) = unsafe { (current as *mut ISDBText).as_mut() } {
            let mut found = false;
            let mut sb_current = ctx.buffered_text.next;
            while let Some(sb_text) = unsafe { (sb_current as *mut ISDBText).as_mut() } {
                if unsafe { ccx_strstr_ignorespace(text.buf, sb_text.buf) } != 0 {
                    found = true;
                    if unsafe { ccx_strstr_ignorespace(sb_text.buf, text.buf) } == 0 {
                        reserve_buf(sb_text, text.used).expect("Failed to reserve buffer space");
                        unsafe {
                            std::ptr::copy_nonoverlapping(text.buf, sb_text.buf, text.used);
                        }
                    }
                    break;
                }
                sb_current = sb_text.list.next;
            }
            if !found {
                let mut sb_text = allocate_text_node(ls).expect("Failed to allocate text node");
                unsafe {
                    // Add to buffered_text
                    sb_text.list.next = &mut ctx.buffered_text as *mut ListHead;
                    sb_text.list.prev = ctx.buffered_text.prev;
                    (*ctx.buffered_text.prev).next = &mut sb_text.list as *mut ListHead;
                    ctx.buffered_text.prev = &mut sb_text.list as *mut ListHead;
                }
                reserve_buf(&mut sb_text, text.used).expect("Failed to reserve buffer space");
                unsafe {
                    std::ptr::copy_nonoverlapping(text.buf, sb_text.buf, text.used);
                    *sb_text.buf.add(text.used) = b'\0' as i8;
                }
                sb_text.used = text.used;
            }
            current = text.list.next;
        }

        // Flush buffered_text if text is not in text_list_head
        let mut sb_current = ctx.buffered_text.next;
        while sb_current != &mut ctx.buffered_text as *mut ListHead {
            let sb_text = unsafe { &mut *(sb_current as *mut ISDBText) };
            let mut found = false;
            let mut current = ctx.text_list_head.next;
            while let Some(text) = unsafe { (current as *mut ISDBText).as_mut() } {
                if unsafe { ccx_strstr_ignorespace(text.buf, sb_text.buf) } != 0 {
                    found = true;
                    break;
                }
                current = text.list.next;
            }
            if !found && len - index > sb_text.used + 2 && sb_text.used > 0 {
                unsafe {
                    std::ptr::copy_nonoverlapping(
                        sb_text.buf as *mut u8,
                        buffer.as_mut_ptr().add(index),
                        sb_text.used,
                    );
                    *buffer.as_mut_ptr().add(index + sb_text.used) = b'\n';
                    *buffer.as_mut_ptr().add(index + sb_text.used + 1) = b'\r';
                }
                index += sb_text.used + 2;
                sb_text.used = 0;

                // Remove from buffered_text
                unsafe {
                    (*sb_text.list.prev).next = sb_text.list.next;
                    (*sb_text.list.next).prev = sb_text.list.prev;
                }
            }
            sb_current = sb_text.list.next;
        }
    } else {
        // Copy text_list_head directly to buffer
        let mut current = ctx.text_list_head.next;
        while let Some(text) = unsafe { (current as *mut ISDBText).as_mut() } {
            if len - index > text.used + 2 && text.used > 0 {
                unsafe {
                    std::ptr::copy_nonoverlapping(
                        text.buf as *mut u8,
                        buffer.as_mut_ptr().add(index),
                        text.used,
                    );
                    *buffer.as_mut_ptr().add(index + text.used) = b'\n';
                    *buffer.as_mut_ptr().add(index + text.used + 1) = b'\r';
                }
                index += text.used + 2;
                text.used = 0;
                unsafe {
                    *text.buf = b'\0' as i8;
                }
            }
            current = text.list.next;
        }
    }
    index
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `arg` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn set_writing_format(ctx: &mut ISDBSubContext, arg: &[u8]) {
    let ls = &mut ctx.current_state.layout_state;

    // One param means its initialization
    if arg.get(1) == Some(&0x20) {
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

    // P1 I1 P2 I2 P31 ~ P3i I3 P41 ~ P4j I4 F
    if arg.get(1) == Some(&0x3B) {
        ls.format = WritingFormat::HorizontalCustom;
        let mut arg_iter = arg.iter().skip(2);

        if let Some(&value) = arg_iter.next() {
            match value & 0x0F {
                0 => {
                    // ctx.font_size = SMALL_FONT_SIZE;
                }
                1 => {
                    // ctx.font_size = MIDDLE_FONT_SIZE;
                }
                2 => {
                    // ctx.font_size = STANDARD_FONT_SIZE;
                }
                _ => {}
            }
        }

        // P3
        info!("character numbers in one line in decimal:");
        for &value in arg_iter.by_ref() {
            if value == 0x3B || value == 0x20 {
                break;
            }
            ctx.nb_char = value as i32;
            print!(" {:x}", value & 0x0F);
        }

        if arg_iter.clone().next() == Some(&0x20) {
            return;
        }

        // P4
        info!("line numbers in decimal:");
        for &value in arg_iter {
            if value == 0x20 {
                break;
            }
            ctx.nb_line = value as i32;
            print!(" {:x}", value & 0x0F);
        }
    }
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - Improper usage of the context may result in undefined behavior.
pub fn move_penpos(ctx: &mut ISDBSubContext, col: i32, row: i32) {
    let ls = &mut ctx.current_state.layout_state;

    ls.cursor_pos.x = row;
    ls.cursor_pos.y = col;
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - Improper usage of the context may result in undefined behavior.
pub fn set_position(ctx: &mut ISDBSubContext, p1: i32, p2: i32) {
    let ls = &mut ctx.current_state.layout_state;
    let (cw, ch, col, row);

    if is_horizontal_layout(ls.format) {
        cw = (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscx / 100;
        ch = (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscy / 100;
        // Pen position is at bottom left
        col = p2 * cw;
        row = p1 * ch + ch;
    } else {
        cw = (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscy / 100;
        ch = (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscx / 100;
        // Pen position is at upper center,
        // but in -90deg rotated coordinates, it is at middle left.
        col = p2 * cw;
        row = p1 * ch + ch / 2;
    }

    move_penpos(ctx, col, row);
}

/// # Safety
///
/// - The caller must ensure that `q` is a valid slice of bytes.
/// - If `p1` or `p2` are provided, they must be valid, mutable references to `u32`.
/// - Improper usage of the slice or references may result in undefined behavior.
pub fn get_csi_params(q: &[u8], p1: Option<&mut u32>, p2: Option<&mut u32>) -> i32 {
    let mut q_iter = q.iter();
    let mut q_pivot = 0;

    if let Some(p1_ref) = p1 {
        *p1_ref = 0;
        for &byte in q_iter.by_ref() {
            if !(0x30..=0x39).contains(&byte) {
                break;
            }
            *p1_ref *= 10;
            *p1_ref += (byte - 0x30) as u32;
            q_pivot += 1;
        }
        if let Some(&byte) = q_iter.next() {
            q_pivot += 1;
            if byte != 0x20 && byte != 0x3B {
                return -1;
            }
        } else {
            return -1;
        }
    }

    if let Some(p2_ref) = p2 {
        *p2_ref = 0;
        for &byte in q_iter {
            if !(0x30..=0x39).contains(&byte) {
                break;
            }
            *p2_ref *= 10;
            *p2_ref += (byte - 0x30) as u32;
            q_pivot += 1;
        }
        q_pivot += 1; // Account for the final increment after the loop
    }

    q_pivot
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn parse_csi(ctx: &mut ISDBSubContext, buf: &[u8]) -> usize {
    let mut arg = [0u8; 10];
    let mut i = 0;
    let ret; // should be mut and init to 0
    let mut p1 = 0;
    let mut p2 = 0;
    let _buf_pivot = buf; // Unused variable
    let state = &mut ctx.current_state;
    let ls = &mut state.layout_state;

    // Copy buf into arg
    let mut buf_iter = buf.iter();
    for &byte in buf_iter.by_ref() {
        if byte == 0x20 {
            break;
        }
        if i >= arg.len() {
            info!("Unexpected CSI {} >= {}", arg.len(), i);
            break;
        }
        arg[i] = byte;
        i += 1;
    }

    // Ignore terminating 0x20 character
    if let Some(&byte) = buf_iter.next() {
        arg[i] = byte;
    }

    // Process the CSI command
    if let Some(&command) = buf_iter.next() {
        match command {
            command if command == CsiCommand::Swf as u8 => {
                info!("Command: CSI: SWF");
                set_writing_format(ctx, &arg[..=i]);
            }
            command if command == CsiCommand::Ccc as u8 => {
                info!("Command: CSI: CCC");
                ret = get_csi_params(&arg[..=i], Some(&mut p1), None);
                if ret > 0 {
                    ls.ccc =
                        IsdbCCComposition::try_from(p1 as u8).unwrap_or(IsdbCCComposition::None);
                }
            }
            command if command == CsiCommand::Sdf as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), Some(&mut p2));
                if ret > 0 {
                    ls.display_area.w = p1 as i32;
                    ls.display_area.h = p2 as i32;
                }
                info!("Command: CSI: SDF (w: {}, h: {})", p1, p2);
            }
            command if command == CsiCommand::Ssm as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), Some(&mut p2));
                if ret > 0 {
                    ls.font_size = p1 as i32;
                }
                info!("Command: CSI: SSM (x: {}, y: {})", p1, p2);
            }
            command if command == CsiCommand::Sdp as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), Some(&mut p2));
                if ret > 0 {
                    ls.display_area.x = p1 as i32;
                    ls.display_area.y = p2 as i32;
                }
                info!("Command: CSI: SDP (x: {}, y: {})", p1, p2);
            }
            command if command == CsiCommand::Rcs as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), None);
                if ret > 0 {
                    ctx.current_state.raster_color = DEFAULT_CLUT
                        [(((ctx.current_state.clut_high_idx as u32) << 4) | p1) as usize];
                }
                info!("Command: CSI: RCS ({})", p1);
            }
            command if command == CsiCommand::Shs as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), None);
                if ret > 0 {
                    ls.cell_spacing.col = p1 as i32;
                }
                info!("Command: CSI: SHS ({})", p1);
            }
            command if command == CsiCommand::Svs as u8 => {
                ret = get_csi_params(&arg[..=i], Some(&mut p1), None);
                if ret > 0 {
                    ls.cell_spacing.row = p1 as i32;
                }
                info!("Command: CSI: SVS ({})", p1);
            }
            command if command == CsiCommand::Acps as u8 => {
                info!("Command: CSI: ACPS");
                ret = get_csi_params(&arg[..=i], Some(&mut p1), Some(&mut p2));
                if ret > 0 {
                    ls.acps[0] = p1 as i32;
                    ls.acps[1] = p2 as i32;
                }
            }
            _ => {
                info!("Command: CSI: Unknown command 0x{:x}", command);
            }
        }
    }

    // Return the number of bytes processed
    buf.len() - buf_iter.as_slice().len()
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn parse_command(ctx: &mut ISDBSubContext, buf: &[u8]) -> usize {
    let _buf_pivot = buf; // Unused variable
    let mut buf_iter = buf.iter();
    let ret; // should be mut and init to 0

    let code_lo = buf[0] & 0x0F;
    let code_hi = (buf[0] & 0xF0) >> 4;

    let state = &mut ctx.current_state;
    let ls = &mut state.layout_state;

    buf_iter.next(); // Move to the next byte

    match code_hi {
        0x00 => match code_lo {
            0x0 => info!("Command: NUL"),
            0x7 => info!("Command: BEL"),
            0x8 => info!("Command: ABP"),
            0x9 => info!("Command: APF"),
            0xA => info!("Command: APD"),
            0xB => info!("Command: APU"),
            0xC => info!("Command: CS clear Screen"),
            0xD => info!("Command: APR"),
            0xE => info!("Command: LS1"),
            0xF => info!("Command: LS0"),
            _ => info!("Command: Unknown"),
        },
        0x01 => match code_lo {
            0x6 => info!("Command: PAPF"),
            0x8 => info!("Command: CAN"),
            0x9 => info!("Command: SS2"),
            0xB => info!("Command: ESC"),
            0xC => {
                info!("Command: APS");
                if let (Some(&p1), Some(&p2)) = (buf_iter.next(), buf_iter.next()) {
                    set_position(ctx, (p1 & 0x3F) as i32, (p2 & 0x3F) as i32);
                }
            }
            0xD => info!("Command: SS3"),
            0xE => info!("Command: RS"),
            0xF => info!("Command: US"),
            _ => info!("Command: Unknown"),
        },
        0x08 => match code_lo {
            0x0..=0x7 => {
                info!(
                    "Command: Forground color (0x{:X})",
                    DEFAULT_CLUT[code_lo as usize]
                );
                state.fg_color = DEFAULT_CLUT[code_lo as usize];
            }
            0x8 => {
                info!("Command: SSZ");
                ls.font_scale.fscx = 50;
                ls.font_scale.fscy = 50;
            }
            0x9 => {
                info!("Command: MSZ");
                ls.font_scale.fscx = 200;
                ls.font_scale.fscy = 200;
            }
            0xA => {
                info!("Command: NSZ");
                ls.font_scale.fscx = 100;
                ls.font_scale.fscy = 100;
            }
            0xB => {
                info!("Command: SZX");
                buf_iter.next();
            }
            _ => info!("Command: Unknown"),
        },
        0x09 => match code_lo {
            0x0 => {
                if let Some(&byte) = buf_iter.next() {
                    match byte & 0xF0 {
                        0x20 => {
                            info!("Command: COL: Set Clut {}", byte & 0x0F);
                            ctx.current_state.clut_high_idx = byte & 0x0F;
                        }
                        0x40 => {
                            info!(
                                "Command: COL: Set Forground 0x{:08X}",
                                DEFAULT_CLUT[(byte & 0x0F) as usize]
                            );
                            ctx.current_state.fg_color = DEFAULT_CLUT[(byte & 0x0F) as usize];
                        }
                        0x50 => {
                            info!(
                                "Command: COL: Set Background 0x{:08X}",
                                DEFAULT_CLUT[(byte & 0x0F) as usize]
                            );
                            ctx.current_state.bg_color = DEFAULT_CLUT[(byte & 0x0F) as usize];
                        }
                        0x60 => {
                            info!(
                                "Command: COL: Set half Forground 0x{:08X}",
                                DEFAULT_CLUT[(byte & 0x0F) as usize]
                            );
                            ctx.current_state.hfg_color = DEFAULT_CLUT[(byte & 0x0F) as usize];
                        }
                        0x70 => {
                            info!(
                                "Command: COL: Set Half Background 0x{:08X}",
                                DEFAULT_CLUT[(byte & 0x0F) as usize]
                            );
                            ctx.current_state.hbg_color = DEFAULT_CLUT[(byte & 0x0F) as usize];
                        }
                        _ => {}
                    }
                }
            }
            0x1 => {
                info!("Command: FLC");
                buf_iter.next();
            }
            0x2 => {
                info!("Command: CDC");
                buf_iter.nth(2); // Skip 3 bytes
            }
            0x3 => {
                info!("Command: POL");
                buf_iter.next();
            }
            0x4 => {
                info!("Command: WMM");
                buf_iter.nth(2); // Skip 3 bytes
            }
            0x5 => {
                info!("Command: MACRO");
                buf_iter.next();
            }
            0x7 => {
                info!("Command: HLC");
                buf_iter.next();
            }
            0x8 => {
                info!("Command: RPC");
                buf_iter.next();
            }
            0x9 => info!("Command: SPL"),
            0xA => info!("Command: STL"),
            0xB => {
                ret = parse_csi(ctx, buf_iter.as_slice());
                buf_iter.nth(ret - 1); // Skip the parsed bytes
            }
            0xD => {
                info!("Command: TIME");
                buf_iter.nth(1); // Skip 2 bytes
            }
            _ => info!("Command: Unknown"),
        },
        _ => {}
    }

    buf.len() - buf_iter.as_slice().len()
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn parse_caption_management_data(ctx: &mut ISDBSubContext, buf: &[u8]) -> usize {
    let _buf_pivot = buf; // Unused variable
    let mut buf_iter = buf.iter();

    // Parse TMD
    if let Some(&byte) = buf_iter.next() {
        ctx.tmd = match byte >> 6 {
            0 => IsdbTmd::Free,
            1 => IsdbTmd::RealTime,
            2 => IsdbTmd::OffsetTime,
            _ => IsdbTmd::Free,
        };
        info!("CC MGMT DATA: TMD: {:?}", ctx.tmd);

        match ctx.tmd {
            IsdbTmd::Free => {
                info!("Playback time is not restricted to synchronize to the clock.");
            }
            IsdbTmd::OffsetTime => {
                //
                // This 36-bit field indicates offset time to add to the playback time when the
                // clock control mode is in offset time mode. Offset time is coded in the
                // order of hour, minute, second and millisecond, using nine 4-bit binary
                //  coded decimals (BCD).
                //
                // +-----------+-----------+---------+--------------+
                // |  hour     |   minute  |   sec   |  millisecond |
                // +-----------+-----------+---------+--------------+
                // |  2 (4bit) | 2 (4bit)  | 2 (4bit)|    3 (4bit)  |
                // +-----------+-----------+---------+--------------+

                // Parse offset time
                if let (
                    Some(&hour_byte),
                    Some(&min_byte),
                    Some(&sec_byte),
                    Some(&milli_byte1),
                    Some(&milli_byte2),
                ) = (
                    buf_iter.next(),
                    buf_iter.next(),
                    buf_iter.next(),
                    buf_iter.next(),
                    buf_iter.next(),
                ) {
                    ctx.offset_time.hour = ((hour_byte >> 4) * 10 + (hour_byte & 0xF)) as i32;
                    ctx.offset_time.min = ((min_byte >> 4) * 10 + (min_byte & 0xF)) as i32;
                    ctx.offset_time.sec = ((sec_byte >> 4) * 10 + (sec_byte & 0xF)) as i32;
                    ctx.offset_time.milli = ((milli_byte1 >> 4) * 100
                        + ((milli_byte1 & 0xF) * 10)
                        + (milli_byte2 & 0xF)) as i32;

                    info!(
                        "CC MGMT DATA: OTD( h:{} m:{} s:{} millis:{} )",
                        ctx.offset_time.hour,
                        ctx.offset_time.min,
                        ctx.offset_time.sec,
                        ctx.offset_time.milli
                    );
                }
            }
            _ => {
                info!(
                    "Playback time is in accordance with the time of the clock, \
                    which is calibrated by clock signal (TDT). Playback time is \
                    given by PTS."
                );
            }
        }
    }

    // Parse number of languages
    if let Some(&nb_lang) = buf_iter.next() {
        ctx.nb_lang = nb_lang as i32;
        info!("CC MGMT DATA: nb languages: {}", ctx.nb_lang);
    }

    // Parse language data
    for _ in 0..ctx.nb_lang {
        if let Some(&lang_byte) = buf_iter.next() {
            info!("CC MGMT DATA: {}", (lang_byte & 0x1F) >> 5);
            ctx.dmf = lang_byte & 0x0F;
            info!("CC MGMT DATA: DMF 0x{:X}", ctx.dmf);

            if ctx.dmf == 0xC || ctx.dmf == 0xD || ctx.dmf == 0xE {
                if let Some(&dc_byte) = buf_iter.next() {
                    ctx.dc = dc_byte;
                    if ctx.dc == 0x00 {
                        info!("Attenuation Due to Rain");
                    }
                }
            }

            if let (Some(&lang1), Some(&lang2), Some(&lang3)) =
                (buf_iter.next(), buf_iter.next(), buf_iter.next())
            {
                info!(
                    "CC MGMT DATA: languages: {}{}{}",
                    lang1 as char, lang2 as char, lang3 as char
                );
            }

            if let Some(&format_byte) = buf_iter.next() {
                info!("CC MGMT DATA: Format: 0x{:X}", format_byte >> 4);
                info!("CC MGMT DATA: TCS: 0x{:X}", (format_byte >> 2) & 0x3);
                ctx.current_state.rollup_mode = ((format_byte & 0x3) != 0) as i32;
                info!(
                    "CC MGMT DATA: Rollup mode: 0x{:X}",
                    ctx.current_state.rollup_mode
                );
            }
        }
    }

    buf.len() - buf_iter.as_slice().len()
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn parse_statement(ctx: &mut ISDBSubContext, buf: &[u8]) -> i32 {
    let _buf_pivot = buf; // Unused variable
    let mut buf_iter = buf.iter();
    let mut ret;

    while buf.len() - buf_iter.as_slice().len() < buf.len() {
        if let Some(&byte) = buf_iter.next() {
            let code = (byte & 0xF0) >> 4;
            let code_lo = byte & 0x0F;

            if code <= 0x1 {
                ret = parse_command(ctx, buf_iter.as_slice());
            }
            // Special case *1(SP)
            else if code == 0x2 && code_lo == 0x0 {
                ret = append_char(ctx, byte as char) as usize;
            }
            // Special case *3(DEL)
            else if code == 0x7 && code_lo == 0xF {
                // TODO: DEL should have block in fg color
                ret = append_char(ctx, byte as char) as usize;
            } else if code <= 0x7 {
                ret = append_char(ctx, byte as char) as usize;
            } else if code <= 0x9 {
                ret = parse_command(ctx, buf_iter.as_slice());
            }
            // Special case *2(10/0)
            else if (code == 0xA && code_lo == 0x0) || (code == 0x0F && code_lo == 0xF) {
                // TODO: handle
                ret = 1; // Placeholder for handling
            } else {
                ret = append_char(ctx, byte as char) as usize;
            }

            // No need to check for `ret < 0` as `ret` is of type `usize`
            // if ret < 0 {
            //     break;
            // }

            buf_iter.nth(ret - 1); // Move the iterator forward by `ret` bytes
        }
    }

    0
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - Improper usage of the context or slice may result in undefined behavior.
pub fn parse_data_unit(ctx: &mut ISDBSubContext, buf: &[u8]) -> i32 {
    let mut buf_iter = buf.iter();

    // Skip the unit separator
    buf_iter.next();

    // Parse unit parameter
    let unit_parameter = if let Some(&byte) = buf_iter.next() {
        byte
    } else {
        return 0; // Return early if no data
    };

    // Parse length (RB24 equivalent)
    if let (Some(&b1), Some(&b2), Some(&b3)) = (buf_iter.next(), buf_iter.next(), buf_iter.next()) {
        let _len = ((b1 as u32) << 16) | ((b2 as u32) << 8) | (b3 as u32);
        // Use `_len` if needed or keep it as a placeholder to suppress the warning
    } else {
        return 0; // Return early if length cannot be parsed
    }

    // Process based on unit parameter
    match unit_parameter {
        0x20 => {
            parse_statement(ctx, buf_iter.as_slice());
        }
        _ => {
            // Handle other cases if needed
        }
    }

    0
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - The `sub` parameter must be a valid, mutable reference to a `CcSubtitle`.
/// - Improper usage of the context, slice, or subtitle may result in undefined behavior.
pub fn parse_caption_statement_data(
    ctx: &mut ISDBSubContext,
    lang_id: i32,
    buf: &[u8],
    sub: &mut CcSubtitle,
) -> i32 {
    // assinn land_id to ulang_id
    let _ulang_id = lang_id; // Unused variable

    let mut buf_iter = buf.iter();
    let mut buffer = [0u8; 1024];

    // Parse TMD
    let tmd = if let Some(&byte) = buf_iter.next() {
        byte >> 6
    } else {
        return -1;
    };

    // Skip timing data if TMD is 1 or 2
    if tmd == 1 || tmd == 2 {
        buf_iter.nth(4); // Skip 5 bytes
    }

    // Parse length (RB24 equivalent)
    let _len = if let (Some(&b1), Some(&b2), Some(&b3)) = // len unused
        (buf_iter.next(), buf_iter.next(), buf_iter.next())
    {
        ((b1 as u32) << 16) | ((b2 as u32) << 8) | (b3 as u32)
    } else {
        return -1;
    };

    // Parse data unit
    let ret = parse_data_unit(ctx, buf_iter.as_slice());
    if ret < 0 {
        return -1;
    }

    // Get text from the context
    let ret = get_text(ctx, &mut buffer, 1024);

    // no need to check for `ret < 0` as `ret` is of type `usize`
    // if ret < 0 {
    //     return CCX_OK as i32;
    // }

    // Copy data if present in the buffer
    if ret > 0 {
        add_cc_sub_text(
            sub,
            std::str::from_utf8(&buffer[..ret]).unwrap_or(""),
            ctx.prev_timestamp as i64,
            ctx.timestamp as i64,
            "NA",
            "ISDB",
            CcxEncodingType::Utf8,
        );
        if sub.start_time == sub.end_time {
            sub.end_time += 2;
        }
        ctx.prev_timestamp = ctx.timestamp;
    }

    0
}

/// # Safety
///
/// - The caller must ensure that `codec_ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - The `buf` slice must be valid and properly initialized.
/// - The `sub` parameter must be a valid, mutable reference to a `CcSubtitle`.
/// - Improper usage of the context, slice, or subtitle may result in undefined behavior.
pub fn isdb_parse_data_group(
    codec_ctx: &mut ISDBSubContext,
    buf: &[u8],
    sub: &mut CcSubtitle,
) -> i32 {
    let _buf_pivot = buf; // Unused variable
    let mut buf_iter = buf.iter();

    // Parse ID
    let id = if let Some(&byte) = buf_iter.next() {
        byte >> 2
    } else {
        return -1;
    };

    // Debug variables (unused in release mode)
    #[cfg(debug_assertions)]
    let _version = id & 2;

    // Skip unused bytes
    buf_iter.nth(2);

    // Parse group size (RB16 equivalent)
    let group_size = if let (Some(&b1), Some(&b2)) = (buf_iter.next(), buf_iter.next()) {
        u16::from_be_bytes([b1, b2]) as usize
    } else {
        return -1;
    };

    info!("ISDB (Data group) group_size {}", group_size);

    // Check and update timestamps
    if codec_ctx.prev_timestamp > codec_ctx.timestamp {
        codec_ctx.prev_timestamp = codec_ctx.timestamp;
    }

    // Process based on ID
    let _ret = match id & 0x0F {
        // ret is unused variable
        0 => {
            // Caption management
            parse_caption_management_data(codec_ctx, buf_iter.as_slice())
        }
        1..=7 => {
            // Caption statement data
            info!("ISDB {} language", id & 0x0F);
            parse_caption_statement_data(codec_ctx, (id & 0x0F) as i32, buf_iter.as_slice(), sub)
                as usize
        }
        _ => {
            // Not allowed in spec
            info!("ISDB: Invalid ID in data group");
            0
        }
    };

    // no need to check for `ret < 0` as `ret` is of type `usize`
    // if ret < 0 {
    //     return -1;
    // }

    // Skip processed group size
    buf_iter.nth(group_size - 1);

    // Skip CRC (2 bytes)
    buf_iter.nth(1);

    (buf.len() - buf_iter.as_slice().len()) as i32
}

/// # Safety
///
/// - The caller must ensure that `dec_ctx` is a valid, mutable reference to a `LibCcDecode`.
/// - The `buf` slice must be valid and properly initialized.
/// - The `sub` parameter must be a valid, mutable reference to a `CcSubtitle`.
/// - Improper usage of the context, slice, or subtitle may result in undefined behavior.
pub fn isdbsub_decode(dec_ctx: &mut LibCcDecode, buf: &[u8], sub: &mut CcSubtitle) -> i32 {
    let mut buf_iter = buf.iter();

    // Check for synchronization byte
    if let Some(&byte) = buf_iter.next() {
        if byte != 0x80 {
            info!("\nNot a Synchronized PES\n");
            return -1;
        }
    } else {
        return -1;
    }

    // Skip private data stream (0xFF)
    buf_iter.next();

    // Parse header end
    let header_end = if let Some(&byte) = buf_iter.next() {
        buf_iter.as_slice().len() - (byte & 0x0F) as usize
    } else {
        return -1;
    };

    buf_iter.next(); // Skip one more byte

    // Skip header bytes
    while buf_iter.as_slice().len() > header_end {
        buf_iter.next();
    }

    // Set rollup configuration
    if let Some(ctx) = unsafe { (dec_ctx.private_data as *mut ISDBSubContext).as_mut() } {
        ctx.cfg_no_rollup = dec_ctx.no_rollup;

        // Parse data group
        let ret = isdb_parse_data_group(ctx, buf_iter.as_slice(), sub);
        if ret < 0 {
            return -1;
        }
    }

    1
}

/// # Safety
///
/// - The caller must ensure that `ctx` is a valid, mutable reference to an `ISDBSubContext`.
/// - Improper usage of the context may result in undefined behavior.
pub fn isdb_set_global_time(ctx: &mut ISDBSubContext, timestamp: u64) -> i32 {
    ctx.timestamp = timestamp;
    CCX_OK as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::ffi::c_char;
    use std::ffi::CString;

    #[test]
    fn test_is_horizontal_layout() {
        assert!(is_horizontal_layout(WritingFormat::HorizontalStdDensity));
        assert!(is_horizontal_layout(WritingFormat::HorizontalHighDensity));
        assert!(is_horizontal_layout(WritingFormat::HorizontalWesternLang));
        assert!(is_horizontal_layout(WritingFormat::Horizontal1920x1080));
        assert!(!is_horizontal_layout(WritingFormat::Vertical960x540));
        assert!(!is_horizontal_layout(WritingFormat::Vertical720x480));
    }

    #[test]
    fn test_layout_get_width() {
        assert_eq!(layout_get_width(WritingFormat::Horizontal960x540), 960);
        assert_eq!(layout_get_width(WritingFormat::Vertical960x540), 960);
        assert_eq!(layout_get_width(WritingFormat::Horizontal720x480), 720);
        assert_eq!(layout_get_width(WritingFormat::Vertical720x480), 720);
    }

    #[test]
    fn test_layout_get_height() {
        assert_eq!(layout_get_height(WritingFormat::Horizontal960x540), 540);
        assert_eq!(layout_get_height(WritingFormat::Vertical960x540), 540);
        assert_eq!(layout_get_height(WritingFormat::Horizontal720x480), 480);
        assert_eq!(layout_get_height(WritingFormat::Vertical720x480), 480);
    }

    #[test]
    fn test_init_layout() {
        let mut layout = ISDBSubLayout {
            font_size: 0,
            display_area: Default::default(),
            font_scale: Default::default(),
            cursor_pos: Default::default(),
            format: WritingFormat::None,
            cell_spacing: Default::default(),
            ccc: Default::default(),
            acps: [0; 2],
        };

        unsafe {
            init_layout(&mut layout);
        }

        assert_eq!(layout.font_size, 36);
        assert_eq!(layout.display_area.x, 0);
        assert_eq!(layout.display_area.y, 0);
        assert_eq!(layout.font_scale.fscx, 100);
        assert_eq!(layout.font_scale.fscy, 100);
    }

    #[test]
    fn test_ccx_strstr_ignorespace_exact_match() {
        let str1 = CString::new("hello").unwrap();
        let str2 = CString::new("hello").unwrap();

        unsafe {
            assert_eq!(ccx_strstr_ignorespace(str1.as_ptr(), str2.as_ptr()), 1);
        }
    }

    #[test]
    fn test_ccx_strstr_ignorespace_no_match() {
        let str1 = CString::new("hello").unwrap();
        let str2 = CString::new("world").unwrap();

        unsafe {
            assert_eq!(ccx_strstr_ignorespace(str1.as_ptr(), str2.as_ptr()), 0);
        }
    }

    #[test]
    fn test_ccx_strstr_ignorespace_empty_strings() {
        let str1 = CString::new("").unwrap();
        let str2 = CString::new("").unwrap();

        unsafe {
            assert_eq!(ccx_strstr_ignorespace(str1.as_ptr(), str2.as_ptr()), 1);
        }
    }

    #[test]
    fn test_ccx_strstr_ignorespace_spaces_only() {
        let str1 = CString::new("     ").unwrap();
        let str2 = CString::new("").unwrap();

        unsafe {
            assert_eq!(ccx_strstr_ignorespace(str1.as_ptr(), str2.as_ptr()), 1);
        }
    }

    #[test]
    fn test_ccx_strstr_ignorespace_mismatched_lengths() {
        let str1 = CString::new("hello").unwrap();
        let str2 = CString::new("hello world").unwrap();

        unsafe {
            assert_eq!(ccx_strstr_ignorespace(str1.as_ptr(), str2.as_ptr()), 0);
        }
    }

    #[test]
    fn test_init_isdb_decoder_success() {
        // Call the function to initialize the decoder
        let isdb_ctx = init_isdb_decoder();

        // Ensure the context is initialized
        assert!(isdb_ctx.is_some());

        // Verify the initialized values
        let ctx = isdb_ctx.unwrap();
        assert_eq!(ctx.prev_timestamp, u64::MAX);
        assert_eq!(ctx.current_state.clut_high_idx, 0);
        assert_eq!(ctx.current_state.rollup_mode, 0);

        // Verify the list heads are properly initialized
        assert_eq!(
            ctx.text_list_head.next,
            &ctx.text_list_head as *const _ as *mut _
        );
        assert_eq!(
            ctx.text_list_head.prev,
            &ctx.text_list_head as *const _ as *mut _
        );
        assert_eq!(
            ctx.buffered_text.next,
            &ctx.buffered_text as *const _ as *mut _
        );
        assert_eq!(
            ctx.buffered_text.prev,
            &ctx.buffered_text as *const _ as *mut _
        );

        // Verify the layout state is initialized
        let layout = &ctx.current_state.layout_state;
        assert_eq!(layout.font_size, 36);
        assert_eq!(layout.display_area.x, 0);
        assert_eq!(layout.display_area.y, 0);
        assert_eq!(layout.font_scale.fscx, 100);
        assert_eq!(layout.font_scale.fscy, 100);
    }

    #[test]
    fn test_init_isdb_decoder_no_crash() {
        // Ensure the function does not crash when called multiple times
        let _ctx1 = init_isdb_decoder();
        let _ctx2 = init_isdb_decoder();
    }

    #[test]
    fn test_allocate_text_node_buffer_allocation() {
        // Create a mock ISDBSubLayout
        let mut layout = ISDBSubLayout {
            font_size: 36,
            display_area: DispArea {
                x: 0,
                y: 0,
                w: 0,
                h: 0,
            },
            font_scale: FScale {
                fscx: 100,
                fscy: 100,
            },
            cursor_pos: ISDBPos { x: 0, y: 0 },
            format: WritingFormat::HorizontalStdDensity,
            cell_spacing: Spacing { col: 0, row: 0 },
            ccc: IsdbCCComposition::None,
            acps: [0; 2],
        };

        // Call the function to allocate a text node
        let mut text_node = allocate_text_node(&mut layout).unwrap();

        // Allocate memory for the buffer
        let mut buf = vec![0u8; 128];
        text_node.buf = buf.as_mut_ptr() as *mut c_char;
        std::mem::forget(buf); // Prevent Rust from deallocating the buffer

        // Verify the buffer is allocated
        assert!(!text_node.buf.is_null());
    }

    #[test]
    fn test_set_writing_format_invalid_format() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    format: WritingFormat::None,
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Call set_writing_format with an invalid format argument
        set_writing_format(&mut ctx, &[0xFF, 0x20]);

        // Verify the writing format remains None
        assert_eq!(ctx.current_state.layout_state.format, WritingFormat::None);
    }

    #[test]
    fn test_move_penpos_horizontal_layout() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    cursor_pos: ISDBPos { x: 0, y: 0 },
                    format: WritingFormat::HorizontalStdDensity,
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Move the pen position
        move_penpos(&mut ctx, 10, 20);

        // Verify the pen position is updated correctly
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 20);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 10);
    }

    #[test]
    fn test_move_penpos_vertical_layout() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    cursor_pos: ISDBPos { x: 0, y: 0 },
                    format: WritingFormat::Vertical960x540,
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Move the pen position
        move_penpos(&mut ctx, 15, 25);

        // Verify the pen position is updated correctly
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 25);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 15);
    }

    #[test]
    fn test_move_penpos_no_change() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    cursor_pos: ISDBPos { x: 5, y: 5 },
                    format: WritingFormat::HorizontalStdDensity,
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Move the pen position to the same coordinates
        move_penpos(&mut ctx, 5, 5);

        // Verify the pen position remains unchanged
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 5);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 5);
    }

    #[test]
    fn test_get_csi_params_invalid_input() {
        let input = b"123x";
        let mut p1 = 0;

        let result = get_csi_params(input, Some(&mut p1), None);

        // Verify the result
        assert_eq!(result, -1); // Invalid character 'x'
    }

    #[test]
    fn test_get_csi_params_partial_input() {
        let input = b"123;";
        let mut p1 = 0;
        let mut p2 = 0;

        let result = get_csi_params(input, Some(&mut p1), Some(&mut p2));

        // Verify the result
        assert_eq!(result, -1); // Missing second parameter
    }

    #[test]
    fn test_parse_command_nul() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Input buffer for the NUL command
        let buf = b"\x00";

        // Call parse_command
        let result = parse_command(&mut ctx, buf);

        // Verify the command is processed
        assert_eq!(result, 1); // NUL command processes 1 byte
    }

    #[test]
    fn test_parse_command_apf() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    cursor_pos: ISDBPos { x: 0, y: 0 },
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Input buffer for the APF (Advance Pen Forward) command
        let buf = b"\x09";

        // Call parse_command
        let result = parse_command(&mut ctx, buf);

        // Verify the command is processed
        assert_eq!(result, 1); // APF command processes 1 byte
    }

    #[test]
    fn test_parse_command_ssz() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    font_scale: FScale {
                        fscx: 100,
                        fscy: 100,
                    },
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Input buffer for the SSZ (Small Size) command
        let buf = b"\x88";

        // Call parse_command
        let result = parse_command(&mut ctx, buf);

        // Verify the font scale is updated
        assert_eq!(result, 1); // SSZ command processes 1 byte
        assert_eq!(ctx.current_state.layout_state.font_scale.fscx, 50);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscy, 50);
    }

    #[test]
    fn test_parse_command_unknown() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Input buffer for an unknown command
        let buf = b"\xFF";

        // Call parse_command
        let result = parse_command(&mut ctx, buf);

        // Verify the command is processed but does not modify the context
        assert_eq!(result, 1); // Unknown command processes 1 byte
    }

    #[test]
    fn test_parse_caption_management_data_free_mode() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            tmd: IsdbTmd::Free,
            nb_lang: 0,
            ..Default::default()
        };

        // Input buffer for free mode
        let buf = b"\x00\x01";

        // Call parse_caption_management_data
        let result = parse_caption_management_data(&mut ctx, buf);

        // Verify the TMD and number of languages
        assert_eq!(result, buf.len());
        assert_eq!(ctx.tmd, IsdbTmd::Free);
        assert_eq!(ctx.nb_lang, 1);
    }

    #[test]
    fn test_parse_caption_management_data_languages() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            nb_lang: 0,
            dmf: 0,
            dc: 0,
            ..Default::default()
        };

        // Input buffer with language data
        let buf = b"\x00\x02\xC1\x45\x4E\x47\x10";

        // Call parse_caption_management_data
        let result = parse_caption_management_data(&mut ctx, buf);

        // Verify the number of languages and language details
        assert_eq!(result, buf.len());
        assert_eq!(ctx.nb_lang, 2);
        assert_eq!(ctx.dmf, 0x01);
        assert_eq!(ctx.dc, 0);
    }

    #[test]
    fn test_parse_caption_management_data_invalid_buffer() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            tmd: IsdbTmd::Free,
            nb_lang: 0,
            ..Default::default()
        };

        // Input buffer with insufficient data
        let buf = b"\x80";

        // Call parse_caption_management_data
        let result = parse_caption_management_data(&mut ctx, buf);

        // Verify that the function processes only the available data
        assert_eq!(result, buf.len());
        assert_eq!(ctx.tmd, IsdbTmd::OffsetTime);
        assert_eq!(ctx.nb_lang, 0); // No languages parsed
    }

    #[test]
    fn test_parse_statement_unknown_code() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            current_state: ISDBSubState {
                layout_state: ISDBSubLayout {
                    ..Default::default()
                },
                ..Default::default()
            },
            ..Default::default()
        };

        // Input buffer for an unknown code
        let buf = b"\xFF";

        // Call parse_statement
        let result = parse_statement(&mut ctx, buf);

        // Verify the unknown code is processed without errors
        assert_eq!(result, 0); // `parse_statement` returns 0 on success
    }

    #[test]
    fn test_parse_data_unit_empty_buffer() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            ..Default::default()
        };

        // Input buffer with no data
        let buf = b"";

        // Call parse_data_unit
        let result = parse_data_unit(&mut ctx, buf);

        // Verify the function handles an empty buffer gracefully
        assert_eq!(result, 0); // No data to process
    }

    #[test]
    fn test_parse_data_unit_invalid_length() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            ..Default::default()
        };

        // Input buffer with an invalid length (less than 3 bytes for RB24)
        let buf = b"\x1F\x20\x00";

        // Call parse_data_unit
        let result = parse_data_unit(&mut ctx, buf);

        // Verify the function handles invalid length gracefully
        assert_eq!(result, 0); // No valid data to process
    }

    #[test]
    fn test_parse_data_unit_unknown_unit_parameter() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            ..Default::default()
        };

        // Input buffer with an unknown unit parameter
        let buf = b"\x1F\xFF\x00\x00\x00";

        // Call parse_data_unit
        let result = parse_data_unit(&mut ctx, buf);

        // Verify the function processes the unknown unit parameter without errors
        assert_eq!(result, 0); // Unknown unit parameter is ignored
    }

    #[test]
    fn test_parse_caption_statement_data_invalid_buffer() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            tmd: IsdbTmd::Free,
            ..Default::default()
        };

        // Create a mock CcSubtitle
        let mut sub = CcSubtitle {
            ..Default::default()
        };

        // Input buffer with insufficient data
        let buf = b"\x80";

        // Call parse_caption_statement_data
        let result = parse_caption_statement_data(&mut ctx, 1, buf, &mut sub);

        // Verify the function handles the invalid buffer gracefully
        assert_eq!(result, -1);
    }

    #[test]
    fn test_isdb_parse_data_group_insufficient_data() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            ..Default::default()
        };

        // Create a mock CcSubtitle
        let mut sub = CcSubtitle {
            ..Default::default()
        };

        // Input buffer with insufficient data
        let buf = b"\x00";

        // Call isdb_parse_data_group
        let result = isdb_parse_data_group(&mut ctx, buf, &mut sub);

        // Verify the function handles insufficient data gracefully
        assert_eq!(result, -1);
    }

    #[test]
    fn test_isdbsub_decode_invalid_sync_byte() {
        // Create a mock LibCcDecode
        let mut dec_ctx = LibCcDecode {
            private_data: Box::into_raw(Box::new(ISDBSubContext {
                cfg_no_rollup: 0,
                ..Default::default()
            })) as *mut _,
            no_rollup: 1,
            ..Default::default()
        };

        // Create a mock CcSubtitle
        let mut sub = CcSubtitle {
            ..Default::default()
        };

        // Input buffer with an invalid synchronization byte
        let buf = b"\x00\xFF\x00\x00\x00\x00\x00\x00";

        // Call isdbsub_decode
        let result = isdbsub_decode(&mut dec_ctx, buf, &mut sub);

        // Verify the function returns an error
        assert_eq!(result, -1);
    }

    #[test]
    fn test_isdbsub_decode_empty_buffer() {
        // Create a mock LibCcDecode
        let mut dec_ctx = LibCcDecode {
            private_data: Box::into_raw(Box::new(ISDBSubContext {
                cfg_no_rollup: 0,
                ..Default::default()
            })) as *mut _,
            no_rollup: 1,
            ..Default::default()
        };

        // Create a mock CcSubtitle
        let mut sub = CcSubtitle {
            ..Default::default()
        };

        // Input buffer with no data
        let buf = b"";

        // Call isdbsub_decode
        let result = isdbsub_decode(&mut dec_ctx, buf, &mut sub);

        // Verify the function returns an error
        assert_eq!(result, -1);
    }

    #[test]
    fn test_isdb_set_global_time_valid_timestamp() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            timestamp: 0,
            ..Default::default()
        };

        // Set a valid timestamp
        let result = isdb_set_global_time(&mut ctx, 123456789);

        // Verify the timestamp is updated correctly
        assert_eq!(result, CCX_OK as i32);
        assert_eq!(ctx.timestamp, 123456789);
    }

    #[test]
    fn test_isdb_set_global_time_zero_timestamp() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            timestamp: 987654321,
            ..Default::default()
        };

        // Set the timestamp to zero
        let result = isdb_set_global_time(&mut ctx, 0);

        // Verify the timestamp is updated correctly
        assert_eq!(result, CCX_OK as i32);
        assert_eq!(ctx.timestamp, 0);
    }

    #[test]
    fn test_isdb_set_global_time_large_timestamp() {
        // Create a mock ISDBSubContext
        let mut ctx = ISDBSubContext {
            timestamp: 0,
            ..Default::default()
        };

        // Set a large timestamp
        let result = isdb_set_global_time(&mut ctx, u64::MAX);

        // Verify the timestamp is updated correctly
        assert_eq!(result, CCX_OK as i32);
        assert_eq!(ctx.timestamp, u64::MAX);
    }
}
