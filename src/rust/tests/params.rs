#[cfg(test)]
pub mod tests {
    #[test]
    fn test_args() {
        let args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "<ARG 1>", ..., "<ARG n>"].into_iter())
                .expect("Failed to parse arguments");

        let mut opt = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        // Assertions according to the arguments passed
        assert!(opt.hardsubx);
        assert_eq!(opt.hardsubx_hue, Some(60.0));
    }
}
