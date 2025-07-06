use crate::fatal;
use crate::util::log::ExitCause;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum BitstreamError {
    #[error("Bitstream has negative length")]
    NegativeLength,
    #[error("Illegal bit position value: {0}")]
    IllegalBitPosition(u8),
    #[error("Argument is greater than maximum bit number (64): {0}")]
    TooManyBits(u32),
    #[error("Data is insufficient for operation")]
    InsufficientData,
}

pub struct BitStreamRust<'a> {
    pub data: &'a [u8],
    pub pos: usize,
    pub bpos: u8,
    pub bits_left: i64,
    pub error: bool,
    pub _i_pos: usize,
    pub _i_bpos: u8,
}

impl<'a> BitStreamRust<'a> {
    /// Create a new bitstream. Empty data is allowed (bits_left = 0).
    pub fn new(data: &'a [u8]) -> Result<Self, BitstreamError> {
        if data.is_empty() {
            return Err(BitstreamError::NegativeLength);
        }

        Ok(Self {
            data,
            pos: 0,
            bpos: 8,
            bits_left: (data.len() as i64) * 8,
            error: false,
            _i_pos: 0,
            _i_bpos: 0,
        })
    }

    /// Peek at next `bnum` bits without advancing. MSB first.
    pub fn next_bits(&mut self, bnum: u32) -> Result<u64, BitstreamError> {
        if bnum > 64 {
            fatal!(cause = ExitCause::Bug; "In next_bits: Argument is greater than the maximum bit number i.e. 64: {}!", bnum);
        }

        // Sanity check - equivalent to (bstr->end - bstr->pos < 0)
        // In Rust: data.len() is equivalent to end, pos is current position
        if self.pos > self.data.len() {
            fatal!(cause = ExitCause::Bug; "In next_bits: Bitstream can not have negative length!");
        }

        // Keep a negative bitstream.bitsleft, but correct it.
        if self.bits_left <= 0 {
            self.bits_left -= bnum as i64;
            return Ok(0);
        }

        // Calculate the remaining number of bits in bitstream after reading.
        // C: bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1) * 8 + bstr->bpos - bnum;
        self.bits_left =
            (self.data.len() as i64 - self.pos as i64 - 1) * 8 + self.bpos as i64 - bnum as i64;
        if self.bits_left < 0 {
            return Ok(0);
        }

        // Special case for reading zero bits. Return zero
        if bnum == 0 {
            return Ok(0);
        }

        let mut vbit = self.bpos as i32;
        let mut vpos = self.pos;
        let mut res = 0u64;
        let mut remaining_bits = bnum;

        if !(1..=8).contains(&vbit) {
            fatal!(cause = ExitCause::Bug; "In next_bits: Illegal bit position value {}!", vbit);
        }

        loop {
            if vpos >= self.data.len() {
                // We should not get here ...
                fatal!(cause = ExitCause::Bug; "In next_bits: Trying to read after end of data ...");
            }

            // C: res |= (*vpos & (0x01 << (vbit - 1)) ? 1 : 0);
            res |= if self.data[vpos] & (0x01 << (vbit - 1)) != 0 {
                1
            } else {
                0
            };
            vbit -= 1;
            remaining_bits -= 1;

            if vbit == 0 {
                vpos += 1;
                vbit = 8;
            }

            if remaining_bits != 0 {
                res <<= 1;
            } else {
                break;
            }
        }

        // Remember the bitstream position
        self._i_bpos = vbit as u8;
        self._i_pos = vpos;

        Ok(res)
    }
    /// Read and commit `bnum` bits. On underflow or zero, returns 0.
    pub fn read_bits(&mut self, bnum: u32) -> Result<u64, BitstreamError> {
        let res = self.next_bits(bnum)?;

        if bnum == 0 || self.bits_left < 0 {
            return Ok(0);
        }

        self.bpos = self._i_bpos;
        self.pos = self._i_pos;

        Ok(res)
    }
    /// Skip `bnum` bits, advancing the position.
    pub fn skip_bits(&mut self, bnum: u32) -> Result<bool, BitstreamError> {
        // Sanity check - equivalent to (bstr->end - bstr->pos < 0)
        // In Rust: data.len() is equivalent to end, pos is current position
        if self.pos > self.data.len() {
            fatal!(cause = ExitCause::Bug; "In skip_bits: bitstream length cannot be negative!");
        }

        // Keep a negative bitstream.bitsleft, but correct it.
        if self.bits_left < 0 {
            self.bits_left -= bnum as i64;
            return Ok(false);
        }

        // Calculate the remaining number of bits in bitstream after reading.
        // C: bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1) * 8 + bstr->bpos - bnum;
        self.bits_left =
            (self.data.len() as i64 - self.pos as i64 - 1) * 8 + self.bpos as i64 - bnum as i64;
        if self.bits_left < 0 {
            return Ok(false);
        }

        // Special case for reading zero bits. Return true
        if bnum == 0 {
            return Ok(true);
        }

        // Handle the bit position arithmetic more carefully
        // C: bstr->bpos -= bnum % 8;
        // C: bstr->pos += bnum / 8;
        let mut new_bpos = self.bpos as i32 - (bnum % 8) as i32;
        let mut new_pos = self.pos + (bnum / 8) as usize;

        // C: if (bstr->bpos < 1) { bstr->bpos += 8; bstr->pos += 1; }
        if new_bpos < 1 {
            new_bpos += 8;
            new_pos += 1;
        }

        self.bpos = new_bpos as u8;
        self.pos = new_pos;

        Ok(true)
    }

    /// Check alignment: true if on next-byte boundary.
    pub fn is_byte_aligned(&self) -> Result<bool, BitstreamError> {
        if self.pos > self.data.len() {
            fatal!(cause = ExitCause::Bug; "In is_byte_aligned: bitstream length can not be negative!");
        }
        let vbit = self.bpos as i32;
        if vbit == 0 || vbit > 8 {
            fatal!(cause = ExitCause::Bug; "In is_byte_aligned: Illegal bit position value {}!", vbit);
        }
        Ok(vbit == 8)
    }

    /// Align to next byte boundary (commit state).
    pub fn make_byte_aligned(&mut self) -> Result<(), BitstreamError> {
        // Sanity check - equivalent to (bstr->end - bstr->pos < 0)
        // In Rust: data.len() is equivalent to end, pos is current position
        if self.pos > self.data.len() {
            fatal!(cause = ExitCause::Bug; "In make_byte_aligned: bitstream length can not be negative!");
        }

        let vbit = self.bpos as i32;

        if vbit == 0 || vbit > 8 {
            fatal!(cause = ExitCause::Bug; "In make_byte_aligned: Illegal bit position value {}!", vbit);
        }

        // Keep a negative bstr->bitsleft, but correct it.
        if self.bits_left < 0 {
            // Pay attention to the bit alignment
            // C: bstr->bitsleft = (bstr->bitsleft - 7) / 8 * 8;
            self.bits_left = (self.bits_left - 7) / 8 * 8;
            return Ok(());
        }

        if self.bpos != 8 {
            self.bpos = 8;
            self.pos += 1;
        }

        // Reset, in case a next_???() function was used before
        // C: bstr->bitsleft = 0LL + 8 * (bstr->end - bstr->pos - 1) + bstr->bpos;
        self.bits_left = 8 * (self.data.len() as i64 - self.pos as i64 - 1) + self.bpos as i64;

        Ok(())
    }

    /// Peek at next `bynum` bytes without advancing.
    /// Errors if not aligned or insufficient data.
    pub fn next_bytes(&mut self, bynum: usize) -> Result<&'a [u8], BitstreamError> {
        // Sanity check - equivalent to (bstr->end - bstr->pos < 0)
        // In Rust: data.len() is equivalent to end, pos is current position
        if self.pos > self.data.len() {
            fatal!(cause = ExitCause::Bug; "In next_bytes: bitstream length can not be negative!");
        }

        // Keep a negative bstr->bitsleft, but correct it.
        if self.bits_left < 0 {
            self.bits_left -= (bynum * 8) as i64;
            return Err(BitstreamError::InsufficientData);
        }

        // C: bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1) * 8 + bstr->bpos - bynum * 8;
        self.bits_left = (self.data.len() as i64 - self.pos as i64 - 1) * 8 + self.bpos as i64
            - (bynum * 8) as i64;

        // C: if (!is_byte_aligned(bstr) || bstr->bitsleft < 0 || bynum < 1)
        if !self.is_byte_aligned()? || self.bits_left < 0 || bynum < 1 {
            return Err(BitstreamError::InsufficientData);
        }

        // Remember the bitstream position
        // C: bstr->_i_bpos = 8;
        // C: bstr->_i_pos = bstr->pos + bynum;
        self._i_bpos = 8;
        self._i_pos = self.pos + bynum;

        // Return slice of the requested bytes
        // C: return bstr->pos;
        if self.pos + bynum <= self.data.len() {
            Ok(&self.data[self.pos..self.pos + bynum])
        } else {
            Err(BitstreamError::InsufficientData)
        }
    }
    /// Read and commit `bynum` bytes.
    pub fn read_bytes(&mut self, bynum: usize) -> Result<&'a [u8], BitstreamError> {
        let res = self.next_bytes(bynum)?;

        // Advance the bitstream when a read was possible
        // C: if (res) { bstr->bpos = bstr->_i_bpos; bstr->pos = bstr->_i_pos; }
        self.bpos = self._i_bpos;
        self.pos = self._i_pos;

        Ok(res)
    }
    /// Return an integer number with "bytes" precision from the current bitstream position.
    /// Allowed "bytes" values are 1,2,4,8.
    /// This function advances the bitstream pointer when "advance" is true.
    /// Numbers come MSB (most significant first).
    pub fn bitstream_get_num(
        &mut self,
        bytes: usize,
        advance: bool,
    ) -> Result<u64, BitstreamError> {
        let bpos = if advance {
            self.read_bytes(bytes)?
        } else {
            self.next_bytes(bytes)?
        };

        match bytes {
            1 | 2 | 4 | 8 => {}
            _ => {
                fatal!(cause = ExitCause::Bug; "In bitstream_get_num: Illegal precision value [{}]!", bytes);
            }
        }

        let mut rval = 0u64;
        for i in 0..bytes {
            // Read backwards - C: unsigned char *ucpos = ((unsigned char *)bpos) + bytes - i - 1;
            let uc = bpos[bytes - i - 1];
            rval = (rval << 8) + uc as u64;
        }

        Ok(rval)
    }

    /// Read unsigned Exp-Golomb code from bitstream
    pub fn read_exp_golomb_unsigned(&mut self) -> Result<u64, BitstreamError> {
        let mut zeros = 0;

        // Count leading zeros
        while self.read_bits(1)? == 0 && self.bits_left >= 0 {
            zeros += 1;
        }

        // Read the remaining bits
        let remaining_bits = self.read_bits(zeros)?;
        let res = ((1u64 << zeros) - 1) + remaining_bits;

        Ok(res)
    }

    /// Read signed Exp-Golomb code from bitstream
    pub fn read_exp_golomb(&mut self) -> Result<i64, BitstreamError> {
        let res = self.read_exp_golomb_unsigned()? as i64;

        // The following function might truncate when res+1 overflows
        // res = (res+1)/2 * (res % 2 ? 1 : -1);
        // Use this:
        // C: res = (res / 2 + (res % 2 ? 1 : 0)) * (res % 2 ? 1 : -1);
        let result =
            (res / 2 + if res % 2 != 0 { 1 } else { 0 }) * if res % 2 != 0 { 1 } else { -1 };

        Ok(result)
    }
    /// Read signed integer with bnum bits length.
    pub fn read_int(&mut self, bnum: u32) -> Result<i64, BitstreamError> {
        let res = self.read_bits(bnum)?;

        // Special case for reading zero bits. Return zero
        if bnum == 0 {
            return Ok(0);
        }

        // C: return (0xFFFFFFFFFFFFFFFFULL << bnum) | res;
        // Sign extend by filling upper bits with 1s
        let result = (0xFFFFFFFFFFFFFFFFu64 << bnum) | res;

        Ok(result as i64)
    }

    /// Return the value with the bit order reversed.
    pub fn reverse8(data: u8) -> u8 {
        let mut res = 0u8;

        for k in 0..8 {
            res <<= 1;
            res |= if data & (0x01 << k) != 0 { 1 } else { 0 };
        }

        res
    }
}
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bitstream_creation() {
        let data = vec![0xFF, 0x00];
        let bs = BitStreamRust::new(&data);
        assert!(bs.is_ok());
    }

    #[test]
    fn test_read_bits() {
        let data = vec![0b10101010];
        let mut bs = BitStreamRust::new(&data).unwrap();

        assert_eq!(bs.read_bits(1).unwrap(), 1);
        assert_eq!(bs.read_bits(1).unwrap(), 0);
        assert_eq!(bs.read_bits(1).unwrap(), 1);
    }

    #[test]
    fn test_byte_alignment() {
        let data = vec![0xFF];
        let mut bs = BitStreamRust::new(&data).unwrap();

        assert!(bs.is_byte_aligned().unwrap());
        bs.read_bits(1).unwrap();
        assert!(!bs.is_byte_aligned().unwrap());
        bs.make_byte_aligned().unwrap();
        assert!(bs.is_byte_aligned().unwrap());
    }

    #[test]
    fn test_multi_bit_reads() {
        // Test data: 0xFF, 0x00 = 11111111 00000000
        let data = [0xFF, 0x00];
        let mut bs = BitStreamRust::new(&data).unwrap();
        assert_eq!(bs.next_bits(4).unwrap(), 0xF); // 1111
        assert_eq!(bs.next_bits(4).unwrap(), 0xF); // 1111
    }

    #[test]
    fn test_cross_byte_boundary() {
        // Test data: 0xF0, 0x0F = 11110000 00001111
        let data = [0xF0, 0x0F];
        let mut bs = BitStreamRust::new(&data).unwrap();

        // Read 6 bits crossing byte boundary: 111100 (should be 0x3C = 60)
        assert_eq!(bs.next_bits(6).unwrap(), 0x3C);

        // Read remaining 10 bits: 0000001111 (should be 0x0F = 15)
    }

    #[test]
    fn test_large_reads() {
        // Test reading up to 64 bits
        let data = [0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88];
        let mut bs = BitStreamRust::new(&data).unwrap();

        // Read all 64 bits at once
        let result = bs.next_bits(64).unwrap();
        let expected = 0xFFEEDDCCBBAA9988u64;
        assert_eq!(result, expected);
    }

    #[test]
    fn test_zero_bits_read() {
        let data = [0xAA];
        let mut bs = BitStreamRust::new(&data).unwrap();

        // Reading 0 bits should return 0 and not advance position
        assert_eq!(bs.next_bits(0).unwrap(), 0);
        assert_eq!(bs._i_pos, 0);

        // Next read should still work normally
        assert_eq!(bs.next_bits(1).unwrap(), 1);
    }

    #[test]
    fn test_insufficient_data() {
        let data = [0xAA]; // Only 8 bits available
        let mut bs = BitStreamRust::new(&data).unwrap();

        // Try to read more bits than available
        let result = bs.next_bits(16).unwrap();
        assert_eq!(result, 0); // Should return 0 when not enough bits
        assert!(bs.bits_left < 0); // bits_left should be negative

        // Subsequent reads should also return 0
        assert_eq!(bs.next_bits(8).unwrap(), 0);
    }

    #[test]
    fn test_negative_bits_left_behavior() {
        let data = [0xFF];
        let mut bs = BitStreamRust::new(&data).unwrap();

        // Exhaust all bits
        bs.next_bits(8).unwrap();

        // Now bits_left should be 0
        assert_eq!(bs.bits_left, 0);

        // Try to read more - should return 0 and make bits_left negative
        assert_eq!(bs.next_bits(4).unwrap(), 0);
        assert_eq!(bs.bits_left, -4);

        // Another read should make it more negative
        assert_eq!(bs.next_bits(2).unwrap(), 0);
        assert_eq!(bs.bits_left, -6);
    }

    #[test]
    fn test_bits_left_calculation() {
        let data = [0xFF, 0xFF, 0xFF]; // 24 bits total
        let mut bs = BitStreamRust::new(&data).unwrap();

        assert_eq!(bs.bits_left, 24);

        bs.next_bits(5).unwrap();
        assert_eq!(bs.bits_left, 19);
    }
}
