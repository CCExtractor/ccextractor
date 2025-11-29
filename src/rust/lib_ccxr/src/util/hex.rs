pub fn hex_to_int(high: char, low: char) -> i32 {
    let h = match high {
        '0'..='9' => high as i32 - '0' as i32,
        'a'..='f' => high as i32 - 'a' as i32 + 10,
        _ => return -1,
    };
    let l = match low {
        '0'..='9' => low as i32 - '0' as i32,
        'a'..='f' => low as i32 - 'a' as i32 + 10,
        _ => return -1,
    };
    h * 16 + l
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_hex_to_int() {
        assert_eq!(hex_to_int('0', '0'), 0);
        assert_eq!(hex_to_int('f', 'f'), 255);
        assert_eq!(hex_to_int('a', '0'), 160);
        assert_eq!(hex_to_int('0', 'a'), 10);
        assert_eq!(hex_to_int('g', '0'), -1);
        assert_eq!(hex_to_int('0', 'g'), -1);
    }
}
