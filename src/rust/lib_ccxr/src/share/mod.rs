//! Module `share` broadcasts subtitle entries over a pub/sub message bus
//! (Nanomsg) and to interoperate with C code via C-compatible FFI bindings.
//!
//! # Overview
//!
//! - CcxSubEntryMessage Represents a single subtitle entry, including timing and text lines.
//! - CcxSubEntries is A collection of `CcxSubEntryMessage` instances generated from a subtitle frame.
//! - CcxShareStatus Enumeration of possible return statuses for share operations.
//! - Ccx_share_ctx Global context holding the Nanomsg socket and stream metadata.
//!
//! ## Features
//!
//! - Initialize and manage a Nanomsg PUB socket for broadcasting messages.
//! - Convert internal `CcSubtitle` frames into one or more [`CcxSubEntryMessage`] instances.
//! - Send packed protobuf messages over the socket, handling lifecycle of messages.
//! - Signal end-of-stream events to subscribers.
//! - Launch external translation processes via system calls.
//!
//! # C-Compatible API
//!
//! All `extern "C"` functions are safe to call from C code and mirror the underlying Rust logic.
//!
//! ## Message Cleanup and Printing
//!| C Function                            | Rust Binding                         |
//!|---------------------------------------|--------------------------------------|
//!| [`ccx_sub_entry_msg_cleanup`]          | [`ccxr_sub_entry_msg_cleanup`]       |
//!| [`ccx_sub_entry_msg_print`]            | [`ccxr_sub_entry_msg_print`]         |
//!| [`ccx_sub_entries_cleanup`]            | [`ccxr_sub_entries_cleanup`]         |
//!| [`ccx_sub_entries_print`]              | [`ccxr_sub_entries_print`]           |
//!
//! ## Service Control
//!| C Function           | Rust Binding                  |
//!|----------------------|-------------------------------|
//!| [`ccx_share_start`]    | [`ccxr_share_start`]          |
//!| [`ccx_share_stop`]     | [`ccxr_share_stop`]           |
//!
//! ## Sending Subtitles
//!| C Function                  | Rust Binding                  |
//!|-----------------------------|-------------------------------|
//!| [`ccx_share_send`]            | [`ccxr_share_send`]           |
//!| [`_ccx_share_send`]           | [`_ccxr_share_send`]          |
//!| [`ccx_share_stream_done`]     | [`ccxr_share_stream_done`]    |
//!| [`_ccx_share_sub_to_entries`] | [`_ccxr_share_sub_to_entries`]|
//!
//! ## Translator Launch
//!| C Function                    | Description                                     |
//!|-------------------------------|-------------------------------------------------|
//!| `ccx_share_launch_translator` | Spawn external `cctranslate` process for translation.
//!
//!# Nanomsg Options
//!
//! - Default URL: `tcp://*:3269` (configurable via `ccx_options.sharing_url`).
//! - Linger option set to infinite to ensure messages are delivered before shutdown.

use crate::share::functions::sharing::{
    ccxr_sub_entries_cleanup, ccxr_sub_entries_print, ccxr_sub_entry_msg_cleanup,
    ccxr_sub_entry_msg_print,
};

pub mod ccxr_sub_entry_message;
pub mod functions;
pub mod tests;
