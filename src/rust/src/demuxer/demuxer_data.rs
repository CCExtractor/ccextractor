use crate::demuxer::common_structs::CcxRational;
use lib_ccxr::common::{BufferdataType, Codec};

#[derive(Debug, Clone)]
pub struct DemuxerData<'a> {
    pub program_number: i32,
    pub stream_pid: i32,
    pub codec: Option<Codec>,           // ccx_code_type maps to Codec
    pub bufferdatatype: BufferdataType, // ccx_bufferdata_type maps to BufferDataType
    pub buffer_data: &'a [u8],          // Replaced raw pointer with slice
    pub buffer_pos: usize, // Current position index in the slice (basically the buffer pointer in C which is replaced by an index to the slice)
    pub rollover_bits: u32, // Tracks PTS rollover
    pub pts: i64,
    pub tb: CcxRational, // Corresponds to ccx_rational
    pub next_stream: *mut DemuxerData<'a>,
    pub next_program: *mut DemuxerData<'a>,
}

pub const CCX_NOPTS: i64 = 0x8000_0000_0000_0000u64 as i64;

impl Default for DemuxerData<'_> {
    fn default() -> Self {
        DemuxerData {
            program_number: -1,
            stream_pid: -1,
            codec: None,                         // CCX_CODEC_NONE
            bufferdatatype: BufferdataType::Pes, // CCX_PES
            buffer_data: &[],                    // empty slice
            buffer_pos: 0,                       // start at 0
            rollover_bits: 0,                    // no rollover yet
            pts: CCX_NOPTS,                      // no PTS
            tb: CcxRational {
                num: 1,
                den: 90_000,
            }, // default timebase
            next_stream: std::ptr::null_mut(),
            next_program: std::ptr::null_mut(),
        }
    }
}
