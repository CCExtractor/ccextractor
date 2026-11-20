# ASan harnesses (focused parser checks)

Small programs for reproducing or regression-testing parser bounds without a full CCExtractor build.

## ITU-T T.35 short SEI header (#2468)

```bash
gcc -O0 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
    -o t35_header_short_test t35_header_short_test.c
./t35_header_short_test
```

`t35_header_short_test.c` checks payloads of length 0, 1, 2, and the 3-byte minimum header under AddressSanitizer.