
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

// The Algorithm for converting CIEXYZ space to CIELAB requires this subroutine
macro_rules! xyz_to_lab_subroutine {
    ($T: ident, $T_n: ident) => {

        if ($T / $T_n) > 0.008856 {
            f32::powf(($T / $T_n), 1.0 / 3.0)
        } else {
            (7.787 * ($T / $T_n)) + (16.0 / 116.0)
        }
    }
}

#[no_mangle]
pub extern "C" fn rgb_to_lab(
    R: f32, G: f32, B: f32,
    L: &mut f32, a: &mut f32, b: &mut f32
)
{


    // First the values are transformed from the RGB -> XYZ Space
    // The Linear Transform can be represented by the matrix:
    // 0.412453 0.357580 0.180423
    // 0.212671 0.715160 0.072169
    // 0.019334 0.119193 0.950227

    // Linear transformation of the vector [R, G, B]:
    let X: f32 = 0.412453 * R + 0.357580 * G + 0.180423 * B;
    let Y: f32 = 0.212671 * R + 0.715160 * G + 0.072169 * B;
    let Z: f32 = 0.019334 * R + 0.119193 * G + 0.950227 * B;

    // The Algorithm requires X_n, Y_n, Z_n
    // These are the X, Y, Z values of some reference White
    // For the CIE standard illuminant D65, these values are:
    // X_n = 95.0489
    // Y_n = 100
    // Z_n = 108.8840
    let X_n: f32 = 95.0489;
    let Y_n: f32 = 100.0;
    let Z_n: f32 = 108.8840;

    *L = 116.0 * xyz_to_lab_subroutine!(Y, Y_n) - 16.0;
    // Note: Many sources give the algorithm for L as the following:
    // L = 116 * (Y/Y_n)^{1/3} - 16 for Y/Y_n > 0.008856
    // L = 903.3 * Y / Y_n otherwise
    // Note that the expression used in this code is equivalent (mathematically)

    *a = 500.0 * (xyz_to_lab_subroutine!(X, X_n) - xyz_to_lab_subroutine!(Y, Y_n));
    *b = 200.0 * (xyz_to_lab_subroutine!(Y, Y_n) - xyz_to_lab_subroutine!(Z, Z_n));
}