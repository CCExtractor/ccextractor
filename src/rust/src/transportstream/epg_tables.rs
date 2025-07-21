use iconv::{decode, IconvError};
use lib_ccxr::info;
use std::fmt;

/// Specific errors for EPG DVB string decoding.
#[derive(Debug)]
pub enum EpgDecodeError {
    /// Input too short to contain a dynamic code‐page header.
    InvalidHeader,
    /// The requested encoding is known invalid (e.g. ISO8859-12).
    UnsupportedEncoding(&'static str),
    /// Low‐level iconv error.
    Iconv(IconvError),
}

impl fmt::Display for EpgDecodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EpgDecodeError::InvalidHeader => write!(f, "Invalid encoding header"),
            EpgDecodeError::UnsupportedEncoding(enc) => {
                write!(f, "Encoding not supported: {enc}")
            }
            EpgDecodeError::Iconv(e) => write!(f, "iconv error: {e}"),
        }
    }
}

impl From<IconvError> for EpgDecodeError {
    fn from(err: IconvError) -> Self {
        EpgDecodeError::Iconv(err)
    }
}

pub fn epg_dvb_decode_string(input: &[u8]) -> Result<String, EpgDecodeError> {
    // Empty input => empty string.
    if input.is_empty() {
        return Ok(String::new());
    }

    let mut data = input;

    // If first byte ≥ 0x20, assume it's already ASCII/UTF‑8.
    if data[0] >= 0x20 {
        let filtered: Vec<u8> = data.iter().cloned().filter(|&b| b <= 127).collect();
        return Ok(String::from_utf8_lossy(&filtered).to_string());
    }

    // Determine encoding tag.
    let encoding: &str = match data[0] {
        0x01 => {
            data = &data[1..];
            "ISO8859-5"
        }
        0x02 => {
            data = &data[1..];
            "ISO8859-6"
        }
        0x03 => {
            data = &data[1..];
            "ISO8859-7"
        }
        0x04 => {
            data = &data[1..];
            "ISO8859-8"
        }
        0x05 => {
            data = &data[1..];
            "ISO8859-9"
        }
        0x06 => {
            data = &data[1..];
            "ISO8859-10"
        }
        0x07 => {
            data = &data[1..];
            "ISO8859-11"
        }
        0x08 => {
            data = &data[1..];
            "ISO8859-12"
        }
        0x09 => {
            data = &data[1..];
            "ISO8859-13"
        }
        0x0a => {
            data = &data[1..];
            "ISO8859-14"
        }
        0x0b => {
            data = &data[1..];
            "ISO8859-15"
        }
        0x10 => {
            // Dynamic code‑page: need at least three bytes for CPN.
            if data.len() < 3 {
                return Err(EpgDecodeError::InvalidHeader);
            }
            let cpn = u16::from_be_bytes([data[1], data[2]]);
            data = &data[3..];
            // Build the label, then try to decode below.
            // We’ll store it in a String and validate.
            let label = format!("ISO8859-{cpn}");
            // Reject known invalid.
            if label == "ISO8859-12" {
                return Err(EpgDecodeError::UnsupportedEncoding("ISO8859-12"));
            }
            // Use a local String and return from the match arm.
            // The decode call below will need to be adjusted to accept String as encoding.
            return match decode(data, &label) {
                Ok(s) => Ok(s),
                Err(e) => {
                    info!(
                        "Warning: EPG_DVB_decode_string: iconv({}) failed: {}\n",
                        label, e
                    );
                    let filtered: Vec<u8> = data.iter().cloned().filter(|&b| b <= 127).collect();
                    Ok(String::from_utf8_lossy(&filtered).to_string())
                }
            };
        }
        0x11 => {
            data = &data[1..];
            "ISO-10646/UTF8"
        }
        0x12 => {
            data = &data[1..];
            "KS_C_5601-1987"
        }
        0x13 => {
            data = &data[1..];
            "GB2312"
        }
        0x14 => {
            data = &data[1..];
            "BIG-5"
        }
        0x15 => {
            data = &data[1..];
            "UTF-8"
        }
        other => {
            info!("Warning: Reserved encoding detected: {:02x}\n", other);
            data = &data[1..];
            "ISO8859-9"
        }
    };

    // Try the high‑level decode. On any error, fall back to ASCII stripping.
    match decode(data, encoding) {
        Ok(s) => Ok(s),
        Err(e) => {
            info!(
                "Warning: EPG_DVB_decode_string: iconv({}) failed: {}\n",
                encoding, e
            );
            let filtered: Vec<u8> = data.iter().cloned().filter(|&b| b <= 127).collect();
            Ok(String::from_utf8_lossy(&filtered).to_string())
        }
    }
}
