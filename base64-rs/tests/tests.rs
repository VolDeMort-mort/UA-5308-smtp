

use smtp_base64::decode::decode;
use smtp_base64::encode::encode;
use smtp_base64::ffi::SMTP_BASE64_MAX_SIZE;
use smtp_base64::stream_decode::StreamDecoder;
use smtp_base64::stream_encode::StreamEncoder;


mod basic_encoder {
    use super::*;

    #[test]
    fn empty_input() {
        let data: Vec<u8> = vec![];
        let encoded_data = encode(&data).unwrap();
        assert_eq!(encoded_data, b"");
    }

    #[test]
    fn one_byte() {
        let data = vec![b'A'];
        let encoded_data = encode(&data).unwrap();
        assert_eq!(encoded_data, b"QQ==");
    }

    #[test]
    fn two_bytes() {
        let data = vec![b'M', b'a'];
        let encoded_data = encode(&data).unwrap();
        assert_eq!(encoded_data, b"TWE=");
    }

    #[test]
    fn three_bytes() {
        let data = vec![b'M', b'a', b'n'];
        let encoded_data = encode(&data).unwrap();
        assert_eq!(encoded_data, b"TWFu");
    }

    #[test]
    fn more_bytes() {
        let data = vec![b'H', b'e', b'l', b'l', b'o'];
        let encoded_data = encode(&data).unwrap();
        assert_eq!(encoded_data, b"SGVsbG8=");
    }

    #[test]
    fn line_break() {
        let data = vec![b'A'; 57];
        let encoded_data = encode(&data).unwrap();

        let mut expected = Vec::new();
        for _ in 0..19 {
            expected.extend_from_slice(b"QUFB");
        }

        assert_eq!(encoded_data, expected);
        assert_eq!(encoded_data.len(), 76);
        assert!(encoded_data.windows(2).position(|w| w == b"\r\n").is_none());
    }

    #[test]
    fn file_size_check() {
        let data = vec![0u8; SMTP_BASE64_MAX_SIZE + 1];
        assert!(encode(&data).is_err());
    }
}


mod basic_decoder {
    use super::*;

    #[test]
    fn simple_str() {
        let encoded = b"TWFu";
        let decoded = decode(encoded).unwrap();
        assert_eq!(decoded, b"Man");
    }

    #[test]
    fn padding() {
        let encoded = b"TWE=";
        let decoded = decode(encoded).unwrap();
        assert_eq!(decoded, b"Ma");
    }

    #[test]
    fn not_valid_symbol() {
        let encoded = b"TWFu$";
        assert!(decode(encoded).is_err());
    }

    #[test]
    fn length_check() {
        let encoded = b"TWF";
        assert!(decode(encoded).is_err());
    }
}


mod stream_encoder {
    use super::*;

    #[test]
    fn one_call() {
        let mut encoder = StreamEncoder::new();
        let data = [b'M', b'a', b'n'];
        encoder.feed(&data).unwrap();
        encoder.finalize();
        let result = encoder.take_output();

        assert_eq!(result, b"TWFu\r\n");
    }

    #[test]
    fn one_byte_recv() {
        let mut encoder = StreamEncoder::new();

        encoder.feed(&[b'M']).unwrap();
        encoder.feed(&[b'a']).unwrap();
        encoder.feed(&[b'n']).unwrap();
        encoder.finalize();
        let result = encoder.take_output();

        assert_eq!(result, b"TWFu\r\n");
    }

    #[test]
    fn line_break() {
        let data = vec![b'A'; 57];

        let mut expected = Vec::new();
        for _ in 0..19 {
            expected.extend_from_slice(b"QUFB");
        }

        let mut encoder = StreamEncoder::new();
        encoder.feed(&data).unwrap();
        encoder.finalize();
        let encoded_data = encoder.take_output();

        assert_eq!(&encoded_data[..76], &expected[..]);
        let pos = encoded_data.windows(2).position(|w| w == b"\r\n");
        assert_eq!(pos, Some(76));
        assert_eq!(encoded_data.len(), 78);
    }
}


mod stream_decoder {
    use super::*;

    #[test]
    fn four_symbols() {
        let mut decoder = StreamDecoder::new();
        decoder.feed(b"T").unwrap();
        decoder.feed(b"W").unwrap();
        decoder.feed(b"F").unwrap();
        decoder.feed(b"u").unwrap();
        decoder.finalize().unwrap();
        let result = decoder.take_output();

        assert_eq!(result, b"Man");
    }

    #[test]
    fn word_wrapping() {
        let mut decoder = StreamDecoder::new();
        let encoded = b"SGVs\r\nbG8=";

        decoder.feed(encoded).unwrap();
        decoder.finalize().unwrap();
        let result = decoder.take_output();

        assert_eq!(result, b"Hello");
    }

    #[test]
    fn not_full_buffer() {
        let mut decoder = StreamDecoder::new();
        decoder.feed(b"TWF").unwrap();
        assert!(decoder.finalize().is_err());
    }
}
