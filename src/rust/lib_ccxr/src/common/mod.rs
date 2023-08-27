//! Provides common types throughout the codebase.
//!
//! # Conversion Guide
//!
//! | From                    | To                         |
//! |-------------------------|----------------------------|
//! | `ccx_output_format`     | [`OutputFormat`]           |
//! | `ccx_avc_nal_types`     | [`AvcNalType`]             |
//! | `ccx_stream_type`       | [`StreamType`]             |
//! | `ccx_mpeg_descriptor`   | [`MpegDescriptor`]         |
//! | `ccx_datasource`        | [`DataSource`]             |
//! | `ccx_stream_mode_enum`  | [`StreamMode`]             |
//! | `ccx_bufferdata_type`   | [`BufferdataType`]         |
//! | `ccx_frame_type`        | [`FrameType`]              |
//! | `ccx_code_type`         | [`Codec`], [`SelectCodec`] |
//! | `cdp_section_type`      | [`CdpSectionType`]         |
//! | `language[NB_LANGUAGE]` | [`Language`]               |

mod constants;

pub use constants::*;
