use palette::{FromColor, Hsv, Lab, Srgb};

#[no_mangle]
pub extern "C" fn rgb_to_hsv(R: f32, G: f32, B: f32, H: &mut f32, S: &mut f32, V: &mut f32) {
    let rgb = Srgb::new(R, G, B);

    let hsv_rep = Hsv::from_color(rgb);

    *H = hsv_rep.hue.to_positive_degrees();
    *S = hsv_rep.saturation;
    *V = hsv_rep.value;
}

#[no_mangle]
pub extern "C" fn rgb_to_lab(R: f32, G: f32, B: f32, L: &mut f32, a: &mut f32, b: &mut f32) {
    let rgb = Srgb::new(R, G, B);

    // This declaration sets the white-point as per the D65 standard
    let lab_rep = Lab::<palette::white_point::D65, f32>::from_color(rgb);

    *L = lab_rep.l;
    *a = lab_rep.a;
    *b = lab_rep.b;
}
