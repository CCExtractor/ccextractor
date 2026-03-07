/// Converts a single hex character to its integer value.
/// Returns None if the character is not valid (0-9, a-f, or A-F).
fn hex_char_to_val(c: char) -> Option<i32> {
    match c {
        '0'..='9' => Some(c as i32 - '0' as i32),
        'a'..='f' => Some(c as i32 - 'a' as i32 + 10),
        'A'..='F' => Some(c as i32 - 'A' as i32 + 10),
        _ => None,
    }
}

/// Converts two hex characters into a combined integer value.
/// Returns None if either character is invalid.
pub fn hex_to_int(high: char, low: char) -> Option<i32> {
    let h = hex_char_to_val(high)?;
    let l = hex_char_to_val(low)?;
    Some(h * 16 + l)
}

pub fn hex_string_to_int(string: &str, len: usize) -> Option<i32> {
    let mut result = 0;
    for c in string.chars().take(len) {
        result = result * 16 + hex_char_to_val(c)?;
    }
    Some(result)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_hex_to_int() {
        assert_eq!(hex_to_int('4', 'f'), Some(79));
        assert_eq!(hex_to_int('0', '0'), Some(0));
        assert_eq!(hex_to_int('f', 'f'), Some(255));
        assert_eq!(hex_to_int('A', 'F'), Some(175));
        assert_eq!(hex_to_int('z', '1'), None);
    }

    #[test]
    fn test_hex_string_to_int() {
        assert_eq!(hex_string_to_int("4f", 2), Some(79));
        assert_eq!(hex_string_to_int("ff", 2), Some(255));
        assert_eq!(hex_string_to_int("00", 2), Some(0));
        assert_eq!(hex_string_to_int("ffff", 4), Some(65535));
        assert_eq!(hex_string_to_int("4f", 1), Some(4));
        assert_eq!(hex_string_to_int("FF", 2), Some(255));
        assert_eq!(hex_string_to_int("zz", 2), None);
    }
}
