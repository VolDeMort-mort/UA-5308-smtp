use crate::table::ALPHABET;

pub(crate) const MAX_FILE_SIZE: usize = 15 * 1024 * 1024;

pub fn encode(data: &[u8]) -> Result<Vec<u8>, &'static str> {
    if data.len() > MAX_FILE_SIZE {
        return Err("File too large");
    }

    let mut output = Vec::new();
    chunk_bytes(data, &mut output);
    Ok(output)
}

fn chunk_bytes(data: &[u8], output: &mut Vec<u8>) {
    let data_size = data.len();
    let mut line_length: usize = 0;
    let mut i = 0;

    while i < data_size {
        let byte0 = data[i];
        let byte1 = if i + 1 < data_size { data[i + 1] } else { 0 };
        let byte2 = if i + 2 < data_size { data[i + 2] } else { 0 };

        let block = (byte0 as u32) << 16 | (byte1 as u32) << 8 | byte2 as u32;

        let char0 = ALPHABET[((block >> 18) & 0x3F) as usize];
        let char1 = ALPHABET[((block >> 12) & 0x3F) as usize];
        let char2 = if i + 1 < data_size {
            ALPHABET[((block >> 6) & 0x3F) as usize]
        } else {
            b'='
        };
        let char3 = if i + 2 < data_size {
            ALPHABET[(block & 0x3F) as usize]
        } else {
            b'='
        };

        output.push(char0);
        output.push(char1);
        output.push(char2);
        output.push(char3);

        line_length += 4;
        let more_data = i + 3 < data_size;

        if line_length >= 76 && more_data {
            output.extend_from_slice(b"\r\n");
            line_length = 0;
        }

        i += 3;
    }
}
