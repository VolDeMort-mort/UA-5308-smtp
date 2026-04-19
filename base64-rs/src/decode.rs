use crate::table::DECODE_TABLE;

#[derive(Debug, PartialEq)]
pub enum DecodeError {

    InvalidSymbol(u8),

    BadLength,

    InvalidCharacter,
}

pub fn decode(encoded: &[u8]) -> Result<Vec<u8>, DecodeError> {

    for &c in encoded {
        let is_upper = c >= b'A' && c <= b'Z';
        let is_lower = c >= b'a' && c <= b'z';
        let is_digit = c >= b'0' && c <= b'9';
        let is_special = c == b'+' || c == b'/' || c == b'=';
        let is_valid = is_upper || is_lower || is_digit || is_special;

        if !is_valid {
            return Err(DecodeError::InvalidSymbol(c));
        }
    }

    if encoded.len() % 4 != 0 {
        return Err(DecodeError::BadLength);
    }

    let mut decoded = Vec::new();
    let mut i = 0;
    while i < encoded.len() {
        let c0 = encoded[i];
        let c1 = encoded[i + 1];
        let c2 = encoded[i + 2];
        let c3 = encoded[i + 3];

        let value0 = DECODE_TABLE[c0 as usize];
        let value1 = DECODE_TABLE[c1 as usize];
        let value2 = if c2 == b'=' { 0 } else { DECODE_TABLE[c2 as usize] };
        let value3 = if c3 == b'=' { 0 } else { DECODE_TABLE[c3 as usize] };

        if value0 < 0
            || value1 < 0
            || (c2 != b'=' && value2 < 0)
            || (c3 != b'=' && value3 < 0)
        {
            return Err(DecodeError::InvalidCharacter);
        }

        let block =
            (value0 as u32) << 18 | (value1 as u32) << 12 | (value2 as u32) << 6 | value3 as u32;

        decoded.push(((block >> 16) & 0xFF) as u8);

        if c2 != b'=' {
            decoded.push(((block >> 8) & 0xFF) as u8);
        }

        if c3 != b'=' {
            decoded.push((block & 0xFF) as u8);
        }

        i += 4;
    }

    Ok(decoded)
}
