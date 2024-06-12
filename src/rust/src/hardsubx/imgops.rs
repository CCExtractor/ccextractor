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

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_rgb_to_hsv() {
        let (mut h, mut s, mut v) = (0.0, 0.0, 0.0);

        // Red (255, 0, 0)
        rgb_to_hsv(255_f32, 0.0, 0.0, &mut h, &mut s, &mut v);
        assert_eq!(h, 0.0);
        assert_eq!(s, 1.0);
        assert_eq!(v, 1.0);

        // Green (0, 255, 0)
        rgb_to_hsv(0.0, 255_f32, 0.0, &mut h, &mut s, &mut v);
        assert_eq!(h, 120.0);
        assert_eq!(s, 1.0);
        assert_eq!(v, 1.0);

        // Blue (0, 0, 255)
        rgb_to_hsv(0.0, 0.0, 255_f32, &mut h, &mut s, &mut v);
        assert_eq!(h, 240.0);
        assert_eq!(s, 1.0);
        assert_eq!(v, 1.0);

        // White (255, 255, 255)
        rgb_to_hsv(255_f32, 255_f32, 255_f32, &mut h, &mut s, &mut v);
        assert_eq!(h, 0.0);
        assert_eq!(s, 0.0);
        assert_eq!(v, 1.0);
    }

    #[test]
    fn test_rgb_to_lab() {
        let (mut l, mut a, mut b) = (0.0, 0.0, 0.0);

        // Red (255, 0, 0)
        rgb_to_lab(1.0, 0.0, 0.0, &mut l, &mut a, &mut b);
        assert_eq!(l.floor(), 53.0);
        assert_eq!(a.floor(), 80.0);
        assert_eq!(b.floor(), 67.0);

        // Green (0, 255, 0)
        rgb_to_lab(0.0, 1.0, 0.0, &mut l, &mut a, &mut b);
        assert_eq!(l.floor(), 87.0);
        assert_eq!(a.floor(), -87.0);
        assert_eq!(b.floor(), 83.0);

        // Blue (0, 0, 255)
        rgb_to_lab(0.0, 0.0, 1.0, &mut l, &mut a, &mut b);
        assert_eq!(l.floor(), 32.0);
        assert_eq!(a.floor(), 79.0);
        assert_eq!(b.floor(), -108.0);

        // White (255, 255, 255)
        rgb_to_lab(1.0, 1.0, 1.0, &mut l, &mut a, &mut b);
        assert_eq!(l.floor(), 100.0);
        assert_eq!(a.floor(), 0.0);
        assert_eq!(b.floor(), 0.0);
    }
}
