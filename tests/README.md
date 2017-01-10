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

If tests fail after your changes, you could try to debug the failed tests.

Run following commands in the `/tests` directory:

```shell
# build test runner (executable file - runtest)
make
# load runtest to the debgger:
gdb runtest

# start under debugger:
(gdb) run

# if segfault occured:
(gdb) where
```

## DEPENDENCIES

Tests are built around this library: [**libcheck**](https://github.com/libcheck/check), here is [**documentation**](https://libcheck.github.io/check/)
