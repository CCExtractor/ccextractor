
#[no_mangle]
pub extern "C" fn rgb_to_hsv(
    R: f32, G: f32, B: f32,
    H: &mut f32, S: &mut f32, V: &mut f32
)
{
    let r: f32 = R / 255.0;
    let g: f32 = G / 255.0;
    let b: f32 = B / 255.0;
    let max: f32 = f32::max(r, f32::max(g, b));
    let min: f32 = f32::min(r, f32::min(g, b));

    let v: f32 = max;

    let mut h = {
        if max == min {
            0.0
        } else if max == r {
            60.0 * ((g - b) / (max - min))
        } else if max == g {
            60.0 * ((g - b) / (max - min) + 120.0)
        } else {
            60.0 * ((g - b) / (max - min) + 240.0)
        }
    };

    let s = {
        match max {
            x if x == 0.0 => 0.0,
            _ => (max - min) / max
        }
    };

    if h < 0.0
    {
        h += 360.0;
    }

    *H = (h as u8).into();
    *S = ((s * 255.0) as u8).into();
    *V = ((v * 255.0) as u8).into();

}

#[no_mangle]
pub extern "C" fn rgb_to_lab(
    R: f32, G: f32, B: f32,
    L: &mut f32, a: &mut f32, b: &mut f32
)
{

    let X: f32 = (0.412453 * R + 0.357580 * G + 0.180423 * B) / (255.0 * 0.950456);
    let Y: f32 = (0.212671 * R + 0.715160 * G + 0.072169 * B) / 255.0;
    let Z: f32 = (0.019334 * R + 0.119193 * G + 0.950227 * B) / (255.0 * 1.08874);

    let fX: f32;
    let fY: f32;
    let fZ: f32;

    if Y > 0.008856
    {
        fY = Y.powf(1.0/3.0);
        *L = 116.0 * fY - 16.0;
    }

    else
    {
        fY = 7.787 * Y + 16.0 / 116.0;
        *L = 903.3 * Y;
    }

    if X > 0.008856{
        fX = X.powf(1.0 / 3.0);
    }
    else{
        fX = 7.787 * X + 16.0 / 116.0;
    }

    if Z > 0.008856 {
        fZ = Z.powf(1.0 / 3.0);
    }
    else{
        fZ = 7.787 * Z + 16.0 / 116.0;
    }

    *a = 500.0 * (fX - fY);
    *b = 200.0 * (fY - fZ);

    if *L < BLACK!()
    {
        *a = *a * fast_math::exp((*L - BLACK!()) / (BLACK!() / 4.0));
        *b = *b * fast_math::exp((*L - BLACK!()) / (BLACK!() / 4.0));
        *L = BLACK!();
    }

    *b = f32::min(*b, YELLOW!());

}