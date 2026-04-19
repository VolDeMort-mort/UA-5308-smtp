use crate::table::DECODE_TABLE;

pub struct StreamDecoder {
    buffer: [u8; 4],
    buffer_size: usize,
    output: Vec<u8>,
}

#[derive(Debug, PartialEq)]
pub enum StreamDecodeError {
    InvalidCharacter,
    InvalidStream,
}

impl StreamDecoder {
    pub fn new() -> Self {
        Self {
            buffer: [0; 4],
            buffer_size: 0,
            output: Vec::new(),
        }
    }

    pub fn take_output(&mut self) -> Vec<u8> {
        std::mem::take(&mut self.output)
    }

    fn decode_part(&mut self, part: &[u8; 4]) -> Result<(), StreamDecodeError> {
        let value0 = DECODE_TABLE[part[0] as usize];
        let value1 = DECODE_TABLE[part[1] as usize];
        let value2 = if part[2] == b'=' {
            0
        } else {
            DECODE_TABLE[part[2] as usize]
        };
        let value3 = if part[3] == b'=' {
            0
        } else {
            DECODE_TABLE[part[3] as usize]
        };

        if value0 < 0 || value1 < 0 || value2 < -2 || value3 < -2 {
            return Err(StreamDecodeError::InvalidCharacter);
        }

        let block =
            (value0 as u32) << 18 | (value1 as u32) << 12 | (value2 as u32) << 6 | value3 as u32;

        self.output.push(((block >> 16) & 0xFF) as u8);

        if part[2] != b'=' {
            self.output.push(((block >> 8) & 0xFF) as u8);
        }
        if part[3] != b'=' {
            self.output.push((block & 0xFF) as u8);
        }

        Ok(())
    }

    pub fn feed(&mut self, data: &[u8]) -> Result<(), StreamDecodeError> {
        for &c in data {
            if c == b'\r' || c == b'\n' || c == b' ' || c == b'\t' {
                continue;
            }

            self.buffer[self.buffer_size] = c;
            self.buffer_size += 1;

            if self.buffer_size == 4 {
                let buf = self.buffer;
                self.decode_part(&buf)?;
                self.buffer_size = 0;
            }
        }

        Ok(())
    }

    pub fn finalize(&mut self) -> Result<(), StreamDecodeError> {
        if self.buffer_size != 0 {
            if self.buffer_size == 4 {
                let buf = self.buffer;
                self.decode_part(&buf)?;
            } else {
                return Err(StreamDecodeError::InvalidStream);
            }
        }
        Ok(())
    }
}
