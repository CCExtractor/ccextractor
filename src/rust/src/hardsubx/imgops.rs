use palette::{FromColor, Srgb, Lab, Hsv};


#[no_mangle]
pub extern "C" fn rgb_to_hsv(
    R: f32, G: f32, B: f32,
    H: &mut f32, S: &mut f32, V: &mut f32
)
{
    let rgb = Srgb::new(R, G, B);

    let hsv_rep = Hsv::from_color(rgb);

    (*H, *S, *V) = (hsv_rep.hue.to_positive_degrees(), hsv_rep.saturation, hsv_rep.value);
}

#[no_mangle]
pub extern "C" fn rgb_to_lab(
    R: f32, G: f32, B: f32,
    L: &mut f32, a: &mut f32, b: &mut f32
)
{
    let rgb = Srgb::new(R, G, B);

    let lab_rep = Lab::<palette::white_point::D65, f32>::from_color(rgb);
    // The above declaration sets the white-point as per the D65 standard

    (*L, *a, *b) = (lab_rep.l, lab_rep.a, lab_rep.b);
}