# UNIT TESTING

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

## DEBUGGING

If tests failed after your changes, you could debug them (almost all flags for this are set in the `tests/Makefile`.

Run:

```shell
# build test runner
make
# load test runner to the debgger:
gdb runner

# run under debugger:
(gdb) run

# on segfault:
(gdb) where
```

## DEPENDENCIES

Tests are built around this library: [**libcheck**](https://github.com/libcheck/check), here is [**documentation**](https://libcheck.github.io/check/)
