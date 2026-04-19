
pub const ALPHABET: [u8; 64] = [
    b'A', b'B', b'C', b'D', b'E', b'F', b'G', b'H',
    b'I', b'J', b'K', b'L', b'M', b'N', b'O', b'P',
    b'Q', b'R', b'S', b'T', b'U', b'V', b'W', b'X',
    b'Y', b'Z', b'a', b'b', b'c', b'd', b'e', b'f',
    b'g', b'h', b'i', b'j', b'k', b'l', b'm', b'n',
    b'o', b'p', b'q', b'r', b's', b't', b'u', b'v',
    b'w', b'x', b'y', b'z', b'0', b'1', b'2', b'3',
    b'4', b'5', b'6', b'7', b'8', b'9', b'+', b'/',
];


pub const DECODE_TABLE: [i32; 256] = {
    let mut t = [-1i32; 256];
    let mut i = 0;
    while i < 64 {
        t[ALPHABET[i] as usize] = i as i32;
        i += 1;
    }
    t[b'=' as usize] = -2;
    t
};
