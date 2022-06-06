
macro_rules! max_f {
    ($a: ident, $b: ident, $c:ident) =>{
        f32::max($a, f32::max($b, $c))
    };
}

macro_rules! min_f {
    ($a: ident, $b: ident, $c:ident) =>{
        f32::min($a, f32::min($b, $c))
    };
}


#[no_mangle]
pub extern "C" fn rgb_to_hsv(
    R: f32, G: f32, B: f32,
    H: &mut f32, S: &mut f32, V: &mut f32
)
{
    let r: f32 = R / 255.0;
    let g: f32 = G / 255.0;
    let b: f32 = B / 255.0;
    let max: f32 = max_f!(r, g, b);
    let min: f32 = min_f!(r, g, b);

    let mut h: f32;
    let s: f32;
    let v: f32 = max;

    if max == 0.0 || max == min {
        s = 0.0;
        h = 0.0;
    }

    else {
        s = (max - min) / max;

        if max == r
        {
            h = 60.0 * ((g - b) / (max - min)) + 0.0;
        }
        else if max == g {
            h = 60.0 * ((b - r) / (max - min)) + 120.0;
        }
        else {
            h = 60.0 * ((r - g) / (max - min)) + 240.0;
        }
    }

    if h < 0.0
    {
        h += 360.0;
    }

    *H = (h as u8).into();
    *S = ((s * 255.0) as u8).into();
    *V = ((v * 255.0) as u8).into();

}