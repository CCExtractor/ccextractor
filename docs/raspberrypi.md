# Installing on a Raspberry Pi

dependencies for ocr mode:

 * libleptonica-dev
 * libtesseract-dev

```bash
sudo apt-get install libleptonica-dev libtesseract-dev
```

Other than this you just need to cd into the `linux` directory and build:

```bash
cd linux
./autogen.sh
./configure
make
```

If you want OCR enabled, pass `--enable-ocr` to `configure` instead:

```bash
cd linux
./autogen.sh
./configure --enable-ocr
make
```
