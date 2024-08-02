# C to Rust Migration Guide

## Porting C Functions to Rust

This guide outlines the process of migrating C functions to Rust while maintaining compatibility with existing C code.

### Step 1: Identify the C Function

First, identify the C function you want to port. For example, let's consider a function named `net_send_cc()` in a file called `networking.c`:

```c
void net_send_cc() {
    // Some C code
}
```

### Step 2: Create a Pure Rust Equivalent

Write an equivalent function in pure Rust within the `lib_ccxr` module:

```rust
fn net_send_cc() {
    // Rust equivalent code to `net_send_cc` function in `networking.c`
}
```

### Step 3: Create a C-Compatible Rust Function

In the `libccxr_exports` module, create a new function that will be callable from C:

```rust
#[no_mangle]
pub extern "C" fn ccxr_net_send_cc() {
    net_send_cc() // Call the pure Rust function
}
```

### Step 4: Declare the Rust Function in C

In the original C file (`networking.c`), declare the Rust function as an external function:

```rust
extern void ccxr_net_send_cc();
```

### Step 5: Modify the Original C Function

Update the original C function to use the Rust implementation when available:

```c
void net_send_cc() {
    #ifndef DISABLE_RUST
        return ccxr_net_send_cc(); // Use the Rust implementation
    #else
        // Original C code
    #endif
}
```

## Rust module system

- `lib_ccxr` crate -> **The Idiomatic Rust layer**

  - Path: `src/rust/lib_ccxr`
  - This layer will contain the migrated idiomatic Rust. It will have complete documentation and tests.

- `libccxr_exports` module -> **The C-like Rust layer**

  - Path: `src/rust/src/libccxr_exports`
  - This layer will have function names the same as defined in C but with the prefix `ccxr_`. These are the functions defined in the `lib_ccx` crate under appropriate modules. And these functions will be provided to the C library.
  - Ex: `extern "C" fn ccxr_<function_name>(<args>) {}`
