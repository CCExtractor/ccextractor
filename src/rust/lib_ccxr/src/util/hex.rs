/// Converts a single lowercase hex character to its integer value.
/// Returns None if the character is not valid (0-9 or a-f).
fn hex_char_to_val(c: char) -> Option<i32> {
    match c {
        '0'..='9' => Some(c as i32 - '0' as i32),
        'a'..='f' => Some(c as i32 - 'a' as i32 + 10),
        _ => None,
    }
}

/// Converts two lowercase hex characters into a combined integer value.
/// Returns None if either character is invalid.
pub fn hex_to_int(high: char, low: char) -> Option<i32> {
    let h = hex_char_to_val(high)?;
    let l = hex_char_to_val(low)?;
    Some(h * 16 + l)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_hex_to_int() {
        assert_eq!(hex_to_int('4', 'f'), Some(79));
        assert_eq!(hex_to_int('0', '0'), Some(0));
        assert_eq!(hex_to_int('f', 'f'), Some(255));
        assert_eq!(hex_to_int('z', '1'), None); // invalid
        assert_eq!(hex_to_int('A', 'F'), None); // uppercase not supported
    }
}
