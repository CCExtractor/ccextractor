# UNIT TESTS

This folder contains a archetype and several unit-tests for CCExtractor

## RUN TESTS

```shell
cd tests
make
```

This will build and run all test-suite.

If you want MORE output:

```shell
DEBUG=1 make
```

Where `DEBUG` is just an environment variable.

## DEPENDENCIES

Tests are built around this library: [**libcheck**](https://github.com/libcheck/check), here is [**documentation**](https://libcheck.github.io/check/)
