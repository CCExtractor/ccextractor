use std::cmp::min;
use thiserror::Error;

/// Trait for converting Rust types to their C counterparts
pub trait CType<T> {
    /// Convert the Rust type to its C counterpart
    ///
    /// # Safety
    /// This function is unsafe because it creates raw pointers and assumes the underlying data
    /// remains valid for the lifetime of the C type. The caller must ensure that:
    /// - The source data outlives the C type
    /// - The pointers created are valid and properly aligned
    /// - The data is not mutably aliased
    unsafe fn to_ctype(&self) -> T;
}

/// Trait for converting C types to their Rust counterparts
pub trait FromC<T> {
    /// Convert the C type to its Rust counterpart
    fn from_c(value: T) -> Self;
}

#[derive(Debug, Error)]
pub enum BitstreamError {
    #[error("Bitstream has negative length")]
    NegativeLength,
    #[error("Illegal bit position value: {0}")]
    IllegalBitPosition(u8),
    #[error("Argument is greater than maximum bit number (64): {0}")]
    TooManyBits(u32),
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CCBitstream {
    pub pos: *mut u8,
    pub bpos: i32,
    pub end: *mut u8,
    pub bitsleft: i64,
    pub error: i32,
    pub _i_pos: *mut u8,
    pub _i_bpos: i32,
}

pub struct Bitstream<'a> {
    pos: &'a [u8],
    bpos: u8,
    bits_left: i64,
    error: i32,
    _i_pos: usize,
    _i_bpos: u8,
}

impl<'a> CType<CCBitstream> for Bitstream<'a> {
    unsafe fn to_ctype(&self) -> CCBitstream {
        CCBitstream {
            pos: self.pos.as_ptr() as *mut u8,
            bpos: self.bpos as i32,
            end: self.pos.as_ptr().add(self.pos.len()) as *mut u8,
            bitsleft: self.bits_left,
            error: self.error,
            _i_pos: self.pos.as_ptr().add(self._i_pos) as *mut u8,
            _i_bpos: self._i_bpos as i32,
        }
    }
}

impl<'a> FromC<CCBitstream> for Bitstream<'a> {
    fn from_c(value: CCBitstream) -> Self {
        unsafe {
            let len = value.end.offset_from(value.pos) as usize;
            let slice = std::slice::from_raw_parts(value.pos, len);
            let i_pos = if value._i_pos.is_null() {
                0
            } else {
                value._i_pos.offset_from(value.pos) as usize
            };

            Bitstream {
                pos: slice,
                bpos: value.bpos as u8,
                bits_left: value.bitsleft,
                error: value.error,
                _i_pos: i_pos,
                _i_bpos: value._i_bpos as u8,
            }
        }
    }
}

impl<'a> Bitstream<'a> {
    pub fn new(data: &'a [u8]) -> Result<Self, BitstreamError> {
        if data.is_empty() {
            return Err(BitstreamError::NegativeLength);
        }

        Ok(Self {
            pos: data,
            bpos: 8,
            bits_left: (data.len() as i64) * 8,
            error: 0,
            _i_pos: 0,
            _i_bpos: 0,
        })
    }

    pub fn next_bits(&mut self, bnum: u32) -> Result<u64, BitstreamError> {
        if bnum > 64 {
            return Err(BitstreamError::TooManyBits(bnum));
        }

        if self.bits_left <= 0 {
            self.bits_left -= bnum as i64;
            return Ok(0);
        }

        // Calculate remaining bits
        self.bits_left =
            (self.pos.len() as i64 - self._i_pos as i64 - 1) * 8 + self.bpos as i64 - bnum as i64;
        if self.bits_left < 0 {
            return Ok(0);
        }

        if bnum == 0 {
            return Ok(0);
        }

        let mut vbit = self.bpos;
        let mut vpos = self._i_pos;
        let mut res = 0u64;
        let mut bits_to_read = bnum;

        if !(1..=8).contains(&vbit) {
            return Err(BitstreamError::IllegalBitPosition(vbit));
        }

        while bits_to_read > 0 {
            if vpos >= self.pos.len() {
                return Err(BitstreamError::NegativeLength);
            }

            let current_byte = self.pos[vpos];
            res |= if (current_byte & (1 << (vbit - 1))) != 0 {
                1
            } else {
                0
            };

            vbit -= 1;
            bits_to_read -= 1;

            if vbit == 0 {
                vpos += 1;
                vbit = 8;
            }

            if bits_to_read > 0 {
                res <<= 1;
            }
        }

        self._i_bpos = vbit;
        self._i_pos = vpos;

        Ok(res)
    }

    pub fn read_bits(&mut self, bnum: u32) -> Result<u64, BitstreamError> {
        let res = self.next_bits(bnum)?;

        if bnum == 0 || self.bits_left < 0 {
            return Ok(0);
        }

        self.bpos = self._i_bpos;

        Ok(res)
    }

    pub fn skip_bits(&mut self, bnum: u32) -> Result<bool, BitstreamError> {
        if self.bits_left < 0 {
            self.bits_left -= bnum as i64;
            return Ok(false);
        }

        self.bits_left =
            (self.pos.len() as i64 - self._i_pos as i64 - 1) * 8 + self.bpos as i64 - bnum as i64;
        if self.bits_left < 0 {
            return Ok(false);
        }

        if bnum == 0 {
            return Ok(true);
        }

        let mut new_bpos = (self.bpos as i32) - (bnum % 8) as i32;
        let mut new_pos = self._i_pos + (bnum / 8) as usize;

        if new_bpos < 1 {
            new_bpos += 8;
            new_pos += 1;
        }

        self.bpos = new_bpos as u8;
        self._i_pos = new_pos;

        Ok(true)
    }

    pub fn is_byte_aligned(&self) -> Result<bool, BitstreamError> {
        if self.bpos == 0 || self.bpos > 8 {
            return Err(BitstreamError::IllegalBitPosition(self.bpos));
        }
        Ok(self.bpos == 8)
    }

    pub fn make_byte_aligned(&mut self) -> Result<(), BitstreamError> {
        let vbit = self.bpos;

        if vbit == 0 || vbit > 8 {
            return Err(BitstreamError::IllegalBitPosition(vbit));
        }

        if self.bits_left < 0 {
            self.bits_left = (self.bits_left - 7) / 8 * 8;
            return Ok(());
        }

        if self.bpos != 8 {
            self.bpos = 8;
            self._i_pos += 1;
        }

        self.bits_left = (self.pos.len() as i64 - self._i_pos as i64 - 1) * 8 + self.bpos as i64;
        Ok(())
    }

    pub fn next_bytes(&mut self, bynum: usize) -> Result<Option<&[u8]>, BitstreamError> {
        if self.bits_left < 0 {
            self.bits_left -= (bynum * 8) as i64;
            return Ok(None);
        }

        self.bits_left = (self.pos.len() as i64 - self._i_pos as i64 - 1) * 8 + self.bpos as i64
            - (bynum * 8) as i64;

        if !self.is_byte_aligned()? || self.bits_left < 0 || bynum < 1 {
            return Ok(None);
        }

        self._i_bpos = 8;
        self._i_pos += bynum;

        Ok(Some(
            &self.pos[self._i_pos..min(self._i_pos + bynum, self.pos.len())],
        ))
    }

    pub fn read_bytes(&mut self, bynum: usize) -> Result<Option<&[u8]>, BitstreamError> {
        if self.bits_left < 0 {
            self.bits_left -= (bynum * 8) as i64;
            return Ok(None);
        }

        if !self.is_byte_aligned()? || bynum < 1 {
            return Ok(None);
        }

        self.bits_left = (self.pos.len() as i64 - self._i_pos as i64 - 1) * 8 + self.bpos as i64
            - (bynum * 8) as i64;
        if self.bits_left < 0 {
            return Ok(None);
        }

        let new_pos = self._i_pos + bynum;
        let slice = &self.pos[self._i_pos..min(new_pos, self.pos.len())];

        self.bpos = 8;
        self._i_pos = new_pos;

        Ok(Some(slice))
    }

    pub fn read_exp_golomb_unsigned(&mut self) -> Result<u64, BitstreamError> {
        let mut zeros = 0;

        while self.read_bits(1)? == 0 && self.bits_left >= 0 {
            zeros += 1;
        }

        let res = (1u64 << zeros) - 1 + self.read_bits(zeros as u32)?;
        Ok(res)
    }

    pub fn read_exp_golomb(&mut self) -> Result<i64, BitstreamError> {
        let res = self.read_exp_golomb_unsigned()? as i64;
        Ok((res / 2 + if res % 2 != 0 { 1 } else { 0 }) * if res % 2 != 0 { 1 } else { -1 })
    }

    pub fn reverse8(data: u8) -> u8 {
        let mut res = 0u8;
        for k in 0..8 {
            res <<= 1;
            res |= (data & (1 << k)) >> k;
        }
        res
    }
}

/// Initialize a new bitstream from raw pointers
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences raw pointers
/// - Creates a slice from raw parts
/// - Assumes the memory between start and end is valid and properly aligned
/// - Assumes the memory remains valid for the lifetime of the returned Bitstream
#[no_mangle]
pub unsafe extern "C" fn ccxr_init_bitstream(
    start: *const u8,
    end: *const u8,
) -> *mut Bitstream<'static> {
    if start.is_null() || end.is_null() {
        return std::ptr::null_mut();
    }

    let len = end.offset_from(start) as usize;
    let slice = std::slice::from_raw_parts(start, len);
    match Bitstream::new(slice) {
        Ok(bs) => Box::into_raw(Box::new(bs)),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Free a bitstream created by ccxr_init_bitstream
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer
/// - Assumes the pointer was created by ccxr_init_bitstream
/// - Assumes no other references to the Bitstream exist
#[no_mangle]
pub unsafe extern "C" fn ccxr_free_bitstream(bs: *mut Bitstream<'static>) {
    if !bs.is_null() {
        drop(Box::from_raw(bs));
    }
}

/// Read bits from a bitstream without advancing the position
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_next_bits(bs: *mut CCBitstream, bnum: u32) -> u64 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    let val = match rust_bs.next_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    let updated_bs = unsafe { rust_bs.to_ctype() };
    bs.pos = updated_bs.pos;
    bs.bpos = updated_bs.bpos;
    bs._i_pos = updated_bs._i_pos;
    bs._i_bpos = updated_bs._i_bpos;
    bs.bitsleft = updated_bs.bitsleft;
    val
}

/// Read bits from a bitstream and advance the position
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_bits(bs: *mut CCBitstream, bnum: u32) -> u64 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    let val = match rust_bs.read_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    let updated_bs = unsafe { rust_bs.to_ctype() };
    bs.pos = updated_bs.pos;
    bs.bpos = updated_bs.bpos;
    bs._i_pos = updated_bs._i_pos;
    bs._i_bpos = updated_bs._i_bpos;
    bs.bitsleft = updated_bs.bitsleft;
    val
}

/// Skip a number of bits in the bitstream
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_skip_bits(bs: *mut CCBitstream, bnum: u32) -> i32 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    let val = match rust_bs.skip_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    let updated_bs = unsafe { rust_bs.to_ctype() };
    bs.pos = updated_bs.pos;
    bs.bpos = updated_bs.bpos;
    bs._i_pos = updated_bs._i_pos;
    bs._i_bpos = updated_bs._i_bpos;
    bs.bitsleft = updated_bs.bitsleft;
    if val {
        1
    } else {
        0
    }
}

/// Check if the bitstream is byte aligned
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
#[no_mangle]
pub unsafe extern "C" fn ccxr_is_byte_aligned(bs: *const CCBitstream) -> i32 {
    let bs = &*bs;
    let rust_bs = Bitstream::from_c(*bs);
    match rust_bs.is_byte_aligned() {
        Ok(val) => {
            if val {
                1
            } else {
                0
            }
        }
        Err(_) => 0,
    }
}

/// Align the bitstream to the next byte boundary
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_make_byte_aligned(bs: *mut CCBitstream) {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    if rust_bs.make_byte_aligned().is_ok() {
        let updated_bs = unsafe { rust_bs.to_ctype() };
        bs.pos = updated_bs.pos;
        bs.bpos = updated_bs.bpos;
        bs._i_pos = updated_bs._i_pos;
        bs._i_bpos = updated_bs._i_bpos;
        bs.bitsleft = updated_bs.bitsleft;
    }
}

/// Get a pointer to the next bytes in the bitstream without advancing
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Returns a raw pointer that must not outlive the bitstream
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_next_bytes(bs: *mut CCBitstream, bynum: usize) -> *const u8 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    let bytes = match rust_bs.next_bytes(bynum) {
        Ok(Some(bytes)) => bytes,
        _ => return std::ptr::null(),
    };
    let bytes_ptr = bytes.as_ptr();
    let updated_bs = unsafe { rust_bs.to_ctype() };
    bs.pos = updated_bs.pos;
    bs.bpos = updated_bs.bpos;
    bs._i_pos = updated_bs._i_pos;
    bs._i_bpos = updated_bs._i_bpos;
    bs.bitsleft = updated_bs.bitsleft;
    bytes_ptr
}

/// Read bytes from the bitstream and advance the position
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Returns a raw pointer that must not outlive the bitstream
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_bytes(bs: *mut CCBitstream, bynum: usize) -> *const u8 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    let bytes = match rust_bs.read_bytes(bynum) {
        Ok(Some(bytes)) => bytes,
        _ => return std::ptr::null(),
    };
    let bytes_ptr = bytes.as_ptr();
    let updated_bs = unsafe { rust_bs.to_ctype() };
    bs.pos = updated_bs.pos;
    bs.bpos = updated_bs.bpos;
    bs._i_pos = updated_bs._i_pos;
    bs._i_bpos = updated_bs._i_bpos;
    bs.bitsleft = updated_bs.bitsleft;
    bytes_ptr
}

/// Read an unsigned Exp-Golomb code from the bitstream
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_exp_golomb_unsigned(bs: *mut CCBitstream) -> u64 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    match rust_bs.read_exp_golomb_unsigned() {
        Ok(val) => {
            let updated_bs = unsafe { rust_bs.to_ctype() };
            bs.pos = updated_bs.pos;
            bs.bpos = updated_bs.bpos;
            bs._i_pos = updated_bs._i_pos;
            bs._i_bpos = updated_bs._i_bpos;
            bs.bitsleft = updated_bs.bitsleft;
            val
        }
        Err(_) => 0,
    }
}

/// Read a signed Exp-Golomb code from the bitstream
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer to CCBitstream
/// - Assumes the pointer and contained data are valid and properly aligned
/// - Modifies the bitstream state
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_exp_golomb(bs: *mut CCBitstream) -> i64 {
    let bs = &mut *bs;
    let mut rust_bs = Bitstream::from_c(*bs);
    match rust_bs.read_exp_golomb() {
        Ok(val) => {
            let updated_bs = unsafe { rust_bs.to_ctype() };
            bs.pos = updated_bs.pos;
            bs.bpos = updated_bs.bpos;
            bs._i_pos = updated_bs._i_pos;
            bs._i_bpos = updated_bs._i_bpos;
            bs.bitsleft = updated_bs.bitsleft;
            val
        }
        Err(_) => 0,
    }
}

/// Reverse the bits in a byte
///
/// # Safety
/// This function is unsafe because it is part of the FFI interface.
/// However, it does not perform any unsafe operations internally.
#[no_mangle]
pub unsafe extern "C" fn ccxr_reverse8(data: u8) -> u8 {
    Bitstream::reverse8(data)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem;

    #[test]
    fn test_bitstream_creation() {
        let data = vec![0xFF, 0x00];
        let bs = Bitstream::new(&data);
        assert!(bs.is_ok());
    }

    #[test]
    fn test_read_bits() {
        let data = vec![0b10101010];
        let mut bs = Bitstream::new(&data).unwrap();

        assert_eq!(bs.read_bits(1).unwrap(), 1);
        assert_eq!(bs.read_bits(1).unwrap(), 0);
        assert_eq!(bs.read_bits(1).unwrap(), 1);
    }

    #[test]
    fn test_byte_alignment() {
        let data = vec![0xFF];
        let mut bs = Bitstream::new(&data).unwrap();

        assert!(bs.is_byte_aligned().unwrap());
        bs.read_bits(1).unwrap();
        assert!(!bs.is_byte_aligned().unwrap());
        bs.make_byte_aligned().unwrap();
        assert!(bs.is_byte_aligned().unwrap());
    }

    // FFI binding tests
    #[test]
    fn test_ffi_next_bits() {
        let data = vec![0b10101010];
        let mut c_bs = CCBitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { ccxr_next_bits(&mut c_bs, 1) }, 1);
    }

    #[test]
    fn test_ffi_read_bits() {
        let data = vec![0b10101010];
        let mut c_bs = CCBitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { ccxr_read_bits(&mut c_bs, 3) }, 0b101);
    }

    #[test]
    fn test_ffi_byte_alignment() {
        let data = vec![0xFF];
        let mut c_bs = CCBitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { ccxr_is_byte_aligned(&c_bs) }, 1);
        unsafe { ccxr_read_bits(&mut c_bs, 1) };
        assert_eq!(unsafe { ccxr_is_byte_aligned(&c_bs) }, 0);
    }

    #[test]
    fn test_ffi_read_bytes() {
        static DATA: [u8; 3] = [0xAA, 0xBB, 0xCC];
        let mut c_bs = CCBitstream {
            pos: DATA.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { DATA.as_ptr().add(DATA.len()) } as *mut u8,
            bitsleft: (DATA.len() as i64) * 8,
            error: 0,
            _i_pos: DATA.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        unsafe {
            let ptr = ccxr_read_bytes(&mut c_bs, 2);
            assert!(!ptr.is_null());
            let b1 = *ptr;
            let b2 = *ptr.add(1);
            assert_eq!([b1, b2], [0xAA, 0xBB]);
        }
    }

    #[test]
    fn test_ffi_exp_golomb() {
        let data = vec![0b10000000];
        let data_ptr = data.as_ptr();
        let mut c_bs = CCBitstream {
            pos: data_ptr as *mut u8,
            bpos: 8,
            end: unsafe { data_ptr.add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data_ptr as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { ccxr_read_exp_golomb_unsigned(&mut c_bs) }, 0);
        mem::drop(data);
    }

    #[test]
    fn test_ffi_reverse8() {
        assert_eq!(unsafe { ccxr_reverse8(0b10101010) }, 0b01010101);
    }

    #[test]
    fn test_ffi_state_updates() {
        let data = vec![0xAA, 0xBB];
        let mut c_bs = CCBitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        unsafe { ccxr_read_bits(&mut c_bs, 4) };
        assert_eq!(c_bs.bpos, 4);
    }
}
