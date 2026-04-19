use crate::encode::MAX_FILE_SIZE;
use crate::table::ALPHABET;

pub struct StreamEncoder {
    left: [u8; 3],
    left_size: usize,
    line_length: usize,
    total_encoded: usize,
    output: Vec<u8>,
}

#[derive(Debug, PartialEq)]
pub enum StreamEncodeError {
    TooLarge,
}

impl StreamEncoder {
    pub fn new() -> Self {
        Self {
            left: [0; 3],
            left_size: 0,
            line_length: 0,
            total_encoded: 0,
            output: Vec::new(),
        }
    }

    pub fn take_output(&mut self) -> Vec<u8> {
        std::mem::take(&mut self.output)
    }

    fn encode_part(&mut self, data: &[u8], size: usize) {
        let byte0 = data[0];
        let byte1 = if size > 1 { data[1] } else { 0 };
        let byte2 = if size > 2 { data[2] } else { 0 };

        let block = (byte0 as u32) << 16 | (byte1 as u32) << 8 | byte2 as u32;

        let chars: [u8; 4] = [
            ALPHABET[((block >> 18) & 0x3F) as usize],
            ALPHABET[((block >> 12) & 0x3F) as usize],
            if size > 1 {
                ALPHABET[((block >> 6) & 0x3F) as usize]
            } else {
                b'='
            },
            if size > 2 {
                ALPHABET[(block & 0x3F) as usize]
            } else {
                b'='
            },
        ];

        for c in chars {
            self.output.push(c);
            self.line_length += 1;
            if self.line_length == 76 {
                self.output.extend_from_slice(b"\r\n");
                self.line_length = 0;
            }
        }
    }

    pub fn feed(&mut self, data: &[u8]) -> Result<(), StreamEncodeError> {
        let size = data.len();

        if self.total_encoded + size > MAX_FILE_SIZE {
            return Err(StreamEncodeError::TooLarge);
        }

        self.total_encoded += size;
        let mut i = 0;

        if self.left_size > 0 {
            while self.left_size < 3 && i < size {
                if self.left_size >= 2 {
                    break;
                }
                self.left[self.left_size] = data[i];
                self.left_size += 1;
                i += 1;
            }

            if self.left_size == 3 {
                let tmp = self.left;
                self.encode_part(&tmp, 3);
                self.left_size = 0;
            }
        }

        while i + 3 <= size {
            self.encode_part(&data[i..], 3);
            i += 3;
        }

        while i < size && self.left_size < 3 {
            self.left[self.left_size] = data[i];
            self.left_size += 1;
            i += 1;
        }

        Ok(())
    }

    pub fn finalize(&mut self) {
        if self.left_size > 0 {
            let tmp = self.left;
            let sz = self.left_size;
            self.encode_part(&tmp, sz);
            self.left_size = 0;
        }

        if self.line_length > 0 {
            self.output.extend_from_slice(b"\r\n");
        }
    }
}
