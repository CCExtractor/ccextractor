use crate::decoder_vbi::exit_codes::*;
use crate::decoder_vbi::functions_vbi_decode::*;
use crate::decoder_vbi::structs_ccdecode::*;
use crate::decoder_vbi::structs_isdb::*;
use crate::decoder_vbi::structs_vbi_decode::*;
use crate::decoder_vbi::structs_xds::*;

pub fn delete_decoder_vbi(arg: &mut Option<*mut CcxDecoderVbiCtx>) {
    if let Some(ctx_ptr) = arg.take() {
        unsafe {
            let ctx = &mut *ctx_ptr;
            vbi_raw_decoder_destroy(&mut ctx.zvbi_decoder);
        }
    }
}

pub fn init_decoder_vbi(cfg: Option<&CcxDecoderVbiCfg>) -> Option<Box<CcxDecoderVbiCtx>> {
    let mut vbi = Box::new(CcxDecoderVbiCtx {
        vbi_decoder_inited: 0,
        zvbi_decoder: VbiRawDecoder::default(),
        vbi_debug_dump: std::ptr::null_mut(),
    });

    vbi_raw_decoder_init(&mut vbi.zvbi_decoder);

    if cfg.is_none() {
        let rd = &mut vbi.zvbi_decoder;
        rd.scanning = 525;
        rd.sampling_format = VbiPixfmt::Yuv420;
        rd.sampling_rate = 13_500_000;
        rd.bytes_per_line = 720;
        rd.offset = (9.7e-6 * 13.5e6) as i32;
        rd.start = [21, 284];
        rd.count = [1, 1];
        rd.interlaced = CCX_TRUE as i32;
        rd.synchronous = CCX_TRUE as i32;
        vbi_raw_decoder_add_services(rd, VBI_SLICED_CAPTION_525, 0);
    }
    Some(vbi)
}

pub fn decode_vbi(
    dec_ctx: &mut LibCcDecode,
    field: u8,
    buffer: &[u8],
    sub: &mut CcSubtitle,
) -> i64 {
    let mut sliced = [VbiSliced {
        id: 0,
        line: 0,
        data: [0; 56],
    }; 52];

    if dec_ctx.vbi_decoder.is_null() {
        dec_ctx.vbi_decoder = init_decoder_vbi(None).map_or(std::ptr::null_mut(), Box::into_raw);
    }

    let n_lines = vbi_raw_decode(
        unsafe { &mut dec_ctx.vbi_decoder.as_mut().unwrap().zvbi_decoder },
        buffer,
        &mut sliced,
    );

    if n_lines > 0 {
        for slice in sliced.iter().take(n_lines as usize) {
            let mut data = [0u8; 3];
            data[0] = if field == 1 { 0x04 } else { 0x05 };
            data[1] = slice.data[0];
            data[2] = slice.data[1];
            do_cb(dec_ctx, &data, sub);
        }
    }

    CCX_OK
}
