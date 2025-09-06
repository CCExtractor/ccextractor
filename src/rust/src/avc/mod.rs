use crate::avc::core::*;
use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use lib_ccxr::common::AvcNalType;
use lib_ccxr::info;

pub mod common_types;
pub mod core;
pub mod nal;
pub mod sei;

pub trait FromCType<T> {
    // remove after demuxer is merged, just import then from `ctorust.rs`
    /// # Safety
    /// This function is unsafe because it uses raw pointers to get data from C types.
    unsafe fn from_ctype(c_value: T) -> Option<Self>
    where
        Self: Sized;
}
impl FromCType<u8> for AvcNalType {
    unsafe fn from_ctype(value: u8) -> Option<AvcNalType> {
        let returnvalue = match value {
            0 => AvcNalType::Unspecified0,
            1 => AvcNalType::CodedSliceNonIdrPicture1,
            2 => AvcNalType::CodedSlicePartitionA,
            3 => AvcNalType::CodedSlicePartitionB,
            4 => AvcNalType::CodedSlicePartitionC,
            5 => AvcNalType::CodedSliceIdrPicture,
            6 => AvcNalType::Sei,
            7 => AvcNalType::SequenceParameterSet7,
            8 => AvcNalType::PictureParameterSet,
            9 => AvcNalType::AccessUnitDelimiter9,
            10 => AvcNalType::EndOfSequence,
            11 => AvcNalType::EndOfStream,
            12 => AvcNalType::FillerData,
            13 => AvcNalType::SequenceParameterSetExtension,
            14 => AvcNalType::PrefixNalUnit,
            15 => AvcNalType::SubsetSequenceParameterSet,
            16 => AvcNalType::Reserved16,
            17 => AvcNalType::Reserved17,
            18 => AvcNalType::Reserved18,
            19 => AvcNalType::CodedSliceAuxiliaryPicture,
            20 => AvcNalType::CodedSliceExtension,
            21 => AvcNalType::Reserved21,
            22 => AvcNalType::Reserved22,
            23 => AvcNalType::Reserved23,
            24 => AvcNalType::Unspecified24,
            25 => AvcNalType::Unspecified25,
            26 => AvcNalType::Unspecified26,
            27 => AvcNalType::Unspecified27,
            28 => AvcNalType::Unspecified28,
            29 => AvcNalType::Unspecified29,
            30 => AvcNalType::Unspecified30,
            31 => AvcNalType::Unspecified31,
            _ => AvcNalType::Unspecified0, // Default fallback
        };
        Some(returnvalue)
    }
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers from C and calls `process_avc`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_process_avc(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    avcbuf: *mut u8,
    avcbuflen: usize,
    sub: *mut cc_subtitle,
) -> usize {
    if avcbuf.is_null() || avcbuflen == 0 {
        return 0;
    }

    // Create a safe slice from the raw pointer
    let avc_slice = std::slice::from_raw_parts_mut(avcbuf, avcbuflen);

    // Call the safe Rust version
    process_avc(&mut *enc_ctx, &mut *dec_ctx, avc_slice, &mut *sub).unwrap_or_else(|e| {
        info!("Error in process_avc: {:?}", e);
        0 // Return 0 to indicate error
    })
}
