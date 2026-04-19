
use std::ptr;

use crate::decode;
use crate::encode;
use crate::stream_decode::StreamDecoder;
use crate::stream_encode::StreamEncoder;


pub const SMTP_BASE64_MAX_SIZE: usize = encode::MAX_FILE_SIZE;

pub const SMTP_BASE64_OK: i32 = 0;

pub const SMTP_BASE64_EBUFSZ: i32 = -1;

pub const SMTP_BASE64_ETOOLARGE: i32 = -2;

pub const SMTP_BASE64_EINVAL: i32 = -3;

pub const SMTP_BASE64_ENULL: i32 = -4;


fn encode_bound(in_len: usize) -> usize {
    if in_len == 0 {
        return 0;
    }
    let base = ((in_len + 2) / 3) * 4;
    let line_breaks = if base > 76 { (base - 1) / 76 } else { 0 };
    base + line_breaks * 2
}

fn decode_bound(in_len: usize) -> usize {
    (in_len / 4) * 3
}

#[unsafe(no_mangle)]
pub extern "C" fn smtp_base64_encode_len(in_len: usize) -> usize {
    encode_bound(in_len)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_base64_encode(
    in_ptr: *const u8,
    in_len: usize,
    out_ptr: *mut u8,
    out_len: *mut usize,
) -> i32 {
    if out_len.is_null() {
        return SMTP_BASE64_ENULL;
    }

    if in_ptr.is_null() && in_len != 0 {
        return SMTP_BASE64_ENULL;
    }

    let input = if in_len == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(in_ptr, in_len) }
    };

    let encoded = match encode::encode(input) {
        Ok(v) => v,
        Err(_) => return SMTP_BASE64_ETOOLARGE,
    };

    let avail = unsafe { *out_len };
    unsafe { *out_len = encoded.len() };

    if out_ptr.is_null() {
        return SMTP_BASE64_OK;
    }

    if avail < encoded.len() {
        return SMTP_BASE64_EBUFSZ;
    }

    unsafe {
        ptr::copy_nonoverlapping(encoded.as_ptr(), out_ptr, encoded.len());
    }
    SMTP_BASE64_OK
}

#[unsafe(no_mangle)]
pub extern "C" fn smtp_base64_decode_len(in_len: usize) -> usize {
    decode_bound(in_len)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_base64_decode(
    in_ptr: *const u8,
    in_len: usize,
    out_ptr: *mut u8,
    out_len: *mut usize,
) -> i32 {
    if out_len.is_null() {
        return SMTP_BASE64_ENULL;
    }

    if in_ptr.is_null() && in_len != 0 {
        return SMTP_BASE64_ENULL;
    }

    let input = if in_len == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(in_ptr, in_len) }
    };

    let decoded = match decode::decode(input) {
        Ok(v) => v,
        Err(_) => return SMTP_BASE64_EINVAL,
    };

    let avail = unsafe { *out_len };
    unsafe { *out_len = decoded.len() };

    if out_ptr.is_null() {
        return SMTP_BASE64_OK;
    }

    if avail < decoded.len() {
        return SMTP_BASE64_EBUFSZ;
    }

    unsafe {
        ptr::copy_nonoverlapping(decoded.as_ptr(), out_ptr, decoded.len());
    }
    SMTP_BASE64_OK
}


pub struct SmtpStreamEncoder(StreamEncoder);

#[unsafe(no_mangle)]
pub extern "C" fn smtp_stream_encoder_new() -> *mut SmtpStreamEncoder {
    Box::into_raw(Box::new(SmtpStreamEncoder(StreamEncoder::new())))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_encoder_free(enc: *mut SmtpStreamEncoder) {
    if !enc.is_null() {
        drop(unsafe { Box::from_raw(enc) });
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_encoder_feed(
    enc: *mut SmtpStreamEncoder,
    data: *const u8,
    len: usize,
) -> i32 {
    if enc.is_null() {
        return SMTP_BASE64_ENULL;
    }
    if data.is_null() && len != 0 {
        return SMTP_BASE64_ENULL;
    }

    let input = if len == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(data, len) }
    };

    let enc = unsafe { &mut *enc };
    match enc.0.feed(input) {
        Ok(()) => SMTP_BASE64_OK,
        Err(_) => SMTP_BASE64_ETOOLARGE,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_encoder_finalize(enc: *mut SmtpStreamEncoder) -> i32 {
    if enc.is_null() {
        return SMTP_BASE64_ENULL;
    }
    let enc = unsafe { &mut *enc };
    enc.0.finalize();
    SMTP_BASE64_OK
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_encoder_take(
    enc: *mut SmtpStreamEncoder,
    out_ptr: *mut u8,
    out_len: *mut usize,
) -> i32 {
    if enc.is_null() || out_len.is_null() {
        return SMTP_BASE64_ENULL;
    }

    let enc = unsafe { &mut *enc };
    let buf = enc.0.take_output();
    let avail = unsafe { *out_len };
    unsafe { *out_len = buf.len() };

    if out_ptr.is_null() {
        // Query mode.
        return SMTP_BASE64_OK;
    }

    if avail < buf.len() {
        return SMTP_BASE64_EBUFSZ;
    }

    unsafe {
        ptr::copy_nonoverlapping(buf.as_ptr(), out_ptr, buf.len());
    }
    SMTP_BASE64_OK
}


pub struct SmtpStreamDecoder(StreamDecoder);

#[unsafe(no_mangle)]
pub extern "C" fn smtp_stream_decoder_new() -> *mut SmtpStreamDecoder {
    Box::into_raw(Box::new(SmtpStreamDecoder(StreamDecoder::new())))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_decoder_free(dec: *mut SmtpStreamDecoder) {
    if !dec.is_null() {
        drop(unsafe { Box::from_raw(dec) });
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_decoder_feed(
    dec: *mut SmtpStreamDecoder,
    data: *const u8,
    len: usize,
) -> i32 {
    if dec.is_null() {
        return SMTP_BASE64_ENULL;
    }
    if data.is_null() && len != 0 {
        return SMTP_BASE64_ENULL;
    }

    let input = if len == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(data, len) }
    };

    let dec = unsafe { &mut *dec };
    match dec.0.feed(input) {
        Ok(()) => SMTP_BASE64_OK,
        Err(_) => SMTP_BASE64_EINVAL,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_decoder_finalize(dec: *mut SmtpStreamDecoder) -> i32 {
    if dec.is_null() {
        return SMTP_BASE64_ENULL;
    }
    let dec = unsafe { &mut *dec };
    match dec.0.finalize() {
        Ok(()) => SMTP_BASE64_OK,
        Err(_) => SMTP_BASE64_EINVAL,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn smtp_stream_decoder_take(
    dec: *mut SmtpStreamDecoder,
    out_ptr: *mut u8,
    out_len: *mut usize,
) -> i32 {
    if dec.is_null() || out_len.is_null() {
        return SMTP_BASE64_ENULL;
    }

    let dec = unsafe { &mut *dec };
    let buf = dec.0.take_output();
    let avail = unsafe { *out_len };
    unsafe { *out_len = buf.len() };

    if out_ptr.is_null() {
        return SMTP_BASE64_OK;
    }

    if avail < buf.len() {
        return SMTP_BASE64_EBUFSZ;
    }

    unsafe {
        ptr::copy_nonoverlapping(buf.as_ptr(), out_ptr, buf.len());
    }
    SMTP_BASE64_OK
}
