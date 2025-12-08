//! Provide structures and types for handling demuxing operations
//!
//! These structures are used to manage stream data, parse Program-Specific Information (PSI) such as PAT (Program Association Table)
//! and PMT (Program Map Table), track program and caption PIDs (Packet Identifiers), and buffer incoming data.
//! The main struct [`CcxDemuxer`] orchestrates the demuxing process, holding the state of the input stream,
//! programs, and individual caption or data streams.
//!
//! Key components include:
//! - Storing program and stream specific information ([`ProgramInfo`], [`CapInfo`], [`PMTEntry`]).
//! - Managing PSI data buffers ([`PSIBuffer`]).
//! - Reporting demuxer statistics and findings ([`CcxDemuxReport`], [`FileReport`]).
//! - Handling different elementary stream types ([`Stream_Type`]) and overall stream properties ([`StreamMode`], [`Codec`]).
//!
//! # Conversion Guide
//!
//! This guide helps map conceptual C-style names or older identifiers to the current Rust structures and constants.
//!
//! | From (Conceptual C-Style/Old Name)                        | To (Rust Equivalent)              |
//! |-----------------------------------------------------------|-----------------------------------|
//! | `ccx_demuxer`                                             | [`CcxDemuxer`]                    |
//! | `ccx_demux_report`                                        | [`CcxDemuxReport`]                |
//! | `file_report`                                             | [`FileReport`]                    |
//! | `program_info`                                            | [`ProgramInfo`]                   |
//! | `cap_info`                                                | [`CapInfo`]                       |
//! | `PSI_buffer`                                              | [`PSIBuffer`]                     |
//! | `PMT_entry`                                               | [`PMTEntry`]                      |
//! | `STREAM_TYPE`                                             | [`Stream_Type`]                   |
//! | `ccx_stream_mp4_box`                                      | [`CcxStreamMp4Box`]               |
//! | `init_demuxer` (function)                                 | [`CcxDemuxer::default()`]         |
//! | `ccx_demuxer_open` (function)                             | [`CcxDemuxer::open()`]            |
//! | `ccx_demuxer_close` (function)                            | [`CcxDemuxer::close()`]           |
//! | `ccx_demuxer_reset` (function)                            | [`CcxDemuxer::reset()`]           |
//! | `ccx_demuxer_isopen` (function)                           | [`CcxDemuxer::is_open()`]         |
//! | `ccx_demuxer_get_file_size` (function)                    | [`CcxDemuxer::get_filesize()`]    |
//! | `ccx_demuxer_print_cfg` (function)                        | [`CcxDemuxer::print_cfg()`]       |
//! | `isValidMP4Box` (function)                                | [`is_valid_mp4_box`]              |

pub mod common_types;
pub mod demux;
pub mod demuxer_data;
pub mod stream_functions;
