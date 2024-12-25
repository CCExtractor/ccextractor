use std::cmp::min;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum BitstreamError {
    #[error("Bitstream has negative length")]
    NegativeLength,
    #[error("Illegal bit position value: {0}")]
    IllegalBitPosition(u8),
    #[error("Argument is greater than maximum bit number (64): {0}")]
    TooManyBits(u32),
}

pub struct Bitstream<'a> {
    pos: &'a [u8],
    current_pos: usize,
    bpos: u8,
    bits_left: i64,
    // Internal state for next_bits
    _i_pos: usize,
    _i_bpos: u8,
}

impl<'a> Bitstream<'a> {
    pub fn new(data: &'a [u8]) -> Result<Self, BitstreamError> {
        if data.is_empty() {
            return Err(BitstreamError::NegativeLength);
        }

        Ok(Self {
            pos: data,
            current_pos: 0,
            bpos: 8,
            bits_left: (data.len() as i64) * 8,
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
        self.bits_left = (self.pos.len() as i64 - self.current_pos as i64 - 1) * 8
            + self.bpos as i64
            - bnum as i64;
        if self.bits_left < 0 {
            return Ok(0);
        }

        if bnum == 0 {
            return Ok(0);
        }

        let mut vbit = self.bpos;
        let mut vpos = self.current_pos;
        let mut res = 0u64;
        let mut bits_to_read = bnum;

        if vbit < 1 || vbit > 8 {
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

        // Remember position for potential read
        self._i_bpos = vbit;
        self._i_pos = vpos;

        Ok(res)
    }

    pub fn read_bits(&mut self, bnum: u32) -> Result<u64, BitstreamError> {
        let res = self.next_bits(bnum)?;

        if bnum == 0 || self.bits_left < 0 {
            return Ok(0);
        }

        // Advance the bitstream
        self.bpos = self._i_bpos;
        self.current_pos = self._i_pos;

        Ok(res)
    }

    pub fn skip_bits(&mut self, bnum: u32) -> Result<bool, BitstreamError> {
        if self.bits_left < 0 {
            self.bits_left -= bnum as i64;
            return Ok(false);
        }

        self.bits_left = (self.pos.len() as i64 - self.current_pos as i64 - 1) * 8
            + self.bpos as i64
            - bnum as i64;
        if self.bits_left < 0 {
            return Ok(false);
        }

        if bnum == 0 {
            return Ok(true);
        }

        let mut new_bpos = (self.bpos as i32) - (bnum % 8) as i32;
        let mut new_pos = self.current_pos + (bnum / 8) as usize;

        if new_bpos < 1 {
            new_bpos += 8;
            new_pos += 1;
        }

        self.bpos = new_bpos as u8;
        self.current_pos = new_pos;

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
            self.current_pos += 1;
        }

        self.bits_left =
            (self.pos.len() as i64 - self.current_pos as i64 - 1) * 8 + self.bpos as i64;
        Ok(())
    }

    pub fn next_bytes(&mut self, bynum: usize) -> Result<Option<&[u8]>, BitstreamError> {
        if self.bits_left < 0 {
            self.bits_left -= (bynum * 8) as i64;
            return Ok(None);
        }

        self.bits_left = (self.pos.len() as i64 - self.current_pos as i64 - 1) * 8
            + self.bpos as i64
            - (bynum * 8) as i64;

        if !self.is_byte_aligned()? || self.bits_left < 0 || bynum < 1 {
            return Ok(None);
        }

        self._i_bpos = 8;
        self._i_pos = self.current_pos + bynum;

        Ok(Some(
            &self.pos[self.current_pos..min(self.current_pos + bynum, self.pos.len())],
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

        self.bits_left = (self.pos.len() as i64 - self.current_pos as i64 - 1) * 8
            + self.bpos as i64
            - (bynum * 8) as i64;
        if self.bits_left < 0 {
            return Ok(None);
        }

        let new_pos = self.current_pos + bynum;
        let slice = &self.pos[self.current_pos..min(new_pos, self.pos.len())];

        self.bpos = 8;
        self.current_pos = new_pos;

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

// FFI exports
#[no_mangle]
pub extern "C" fn ccxr_init_bitstream(start: *const u8, end: *const u8) -> *mut Bitstream<'static> {
    if start.is_null() || end.is_null() {
        return std::ptr::null_mut();
    }

    unsafe {
        let len = end.offset_from(start) as usize;
        let slice = std::slice::from_raw_parts(start, len);
        match Bitstream::new(slice) {
            Ok(bs) => Box::into_raw(Box::new(bs)),
            Err(_) => std::ptr::null_mut(),
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_free_bitstream(bs: *mut Bitstream<'static>) {
    if !bs.is_null() {
        unsafe {
            drop(Box::from_raw(bs));
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_next_bits(bs: *mut Bitstream<'static>, bnum: u32) -> u64 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).next_bits(bnum) {
            Ok(val) => val,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_read_bits(bs: *mut Bitstream<'static>, bnum: u32) -> u64 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).read_bits(bnum) {
            Ok(val) => val,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_skip_bits(bs: *mut Bitstream<'static>, bnum: u32) -> i32 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).skip_bits(bnum) {
            Ok(true) => 1,
            Ok(false) => 0,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_is_byte_aligned(bs: *const Bitstream<'static>) -> i32 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).is_byte_aligned() {
            Ok(true) => 1,
            Ok(false) => 0,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_make_byte_aligned(bs: *mut Bitstream<'static>) {
    if !bs.is_null() {
        unsafe {
            let _ = (*bs).make_byte_aligned();
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_next_bytes(bs: *mut Bitstream<'static>, bynum: usize) -> *const u8 {
    if bs.is_null() {
        return std::ptr::null();
    }
    unsafe {
        match (*bs).next_bytes(bynum) {
            Ok(Some(bytes)) => bytes.as_ptr(),
            _ => std::ptr::null(),
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_read_bytes(bs: *mut Bitstream<'static>, bynum: usize) -> *const u8 {
    if bs.is_null() {
        return std::ptr::null();
    }
    unsafe {
        match (*bs).read_bytes(bynum) {
            Ok(Some(bytes)) => bytes.as_ptr(),
            _ => std::ptr::null(),
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_read_exp_golomb_unsigned(bs: *mut Bitstream<'static>) -> u64 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).read_exp_golomb_unsigned() {
            Ok(val) => val,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_read_exp_golomb(bs: *mut Bitstream<'static>) -> i64 {
    if bs.is_null() {
        return 0;
    }
    unsafe {
        match (*bs).read_exp_golomb() {
            Ok(val) => val,
            Err(_) => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn ccxr_reverse8(data: u8) -> u8 {
    Bitstream::reverse8(data)
}

#[cfg(test)]
mod tests {
    use super::*;

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
}
