# Installing on a Raspberry Pi

dependencies for ocr mode:

 * libleptonica-dev
 * libtesseract-dev

```bash
sudo apt-get install libleptonica-dev libtesseract-dev
```

Other than this you just need to cd into the linux directory and run `make` or `make ENABLE_OCR=yes` if you want ocr enabled.
