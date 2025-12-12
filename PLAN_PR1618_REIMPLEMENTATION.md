# Implementation Plan: Reimplement PR #1618 (Issue #1499)

## Current Status

| Phase | Status | Branch | PR |
|-------|--------|--------|-----|
| Phase 1: Rust Core | ✅ COMPLETE | `fix/1499-dtvcc-persistent-state` | #1782 |
| Phase 2: C Headers | ✅ COMPLETE | `fix/1499-dtvcc-persistent-state` | #1782 |
| Phase 3: C Implementation | ✅ COMPLETE | `fix/1499-dtvcc-persistent-state` | #1782 |
| Phase 4: Testing | 🔲 NOT STARTED | - | - |

---

## Problem Statement

The Rust CEA-708 decoder has a fundamental bug: the `Dtvcc` struct is recreated on every call to `ccxr_process_cc_data()`, causing all state to be reset. This breaks stateful caption processing.

**Current buggy code** (`src/rust/src/lib.rs:175-176`):
```rust
let dtvcc_ctx = unsafe { &mut *dec_ctx.dtvcc };
let mut dtvcc = Dtvcc::new(dtvcc_ctx);  // Created fresh EVERY call!
```

## Solution Overview

Create a persistent Rust `Dtvcc` instance stored in `lib_cc_decode.dtvcc_rust` that is:
1. Initialized once at program start via `ccxr_dtvcc_init()`
2. Used throughout processing via pointer access
3. Freed at program end via `ccxr_dtvcc_free()`

---

## Commit-by-Commit Analysis

### Commit 1: `2a1fce53` - Change `Dtvcc::new()` signature
**Status: NEEDED**

Changes `Dtvcc::new()` to take `ccx_decoder_dtvcc_settings` instead of `dtvcc_ctx`.

**Why needed:** The current implementation wraps the C `dtvcc_ctx`, but we need a standalone Rust struct that owns its data and doesn't depend on the C context.

**Files:** `src/rust/src/decoder/mod.rs`

**Key changes:**
- Change `Dtvcc::new(ctx: &'a mut dtvcc_ctx)` to `Dtvcc::new(opts: &ccx_decoder_dtvcc_settings)`
- Change `decoders` from `Vec<&'a mut dtvcc_service_decoder>` to `[Option<Box<dtvcc_service_decoder>>; CCX_DTVCC_MAX_SERVICES]`
- Change `encoder` from `&'a mut encoder_ctx` to `*mut encoder_ctx` (set later)
- Add `pub const CCX_DTVCC_MAX_SERVICES: usize = 63;`

---

### Commit 2: `05c6a2f7` - Create `ccxr_dtvcc_init` & `ccxr_dtvcc_free`
**Status: NEEDED**

FFI functions to create and destroy the Rust Dtvcc from C code.

**Files:** `src/rust/src/lib.rs`

**Key changes:**
```rust
#[no_mangle]
extern "C" fn ccxr_dtvcc_init<'a>(opts_ptr: *const ccx_decoder_dtvcc_settings) -> *mut Dtvcc<'a>

#[no_mangle]
extern "C" fn ccxr_dtvcc_free(dtvcc_rust: *mut Dtvcc)
```

---

### Commit 3: `af23301b` - Create `ccxr_dtvcc_set_encoder`
**Status: NEEDED**

Allows setting the encoder on the Rust Dtvcc after initialization.

**Files:** `src/rust/src/lib.rs`

**Key changes:**
```rust
#[no_mangle]
extern "C" fn ccxr_dtvcc_set_encoder(dtvcc_rust: *mut Dtvcc, encoder: *mut encoder_ctx)
```

---

### Commit 4: `92cc3c54` - Add `dtvcc_rust` member to `lib_cc_decode`
**Status: NEEDED**

Storage for the Rust Dtvcc pointer in C struct.

**Files:** `src/lib_ccx/ccx_decoders_structs.h`

**Key changes:**
```c
struct lib_cc_decode {
    // ... existing fields ...
    void *dtvcc_rust;  // For storing Dtvcc coming from rust
    dtvcc_ctx *dtvcc;
    // ...
};
```

---

### Commit 5: `b9215f59` - Declare extern functions in C header
**Status: NEEDED**

C header declarations for the Rust FFI functions.

**Files:** `src/lib_ccx/ccx_dtvcc.h`

**Key changes:**
```c
#ifndef DISABLE_RUST
extern void *ccxr_dtvcc_init(struct ccx_decoder_dtvcc_settings *settings_dtvcc);
extern void ccxr_dtvcc_free(void *dtvcc_rust);
extern void ccxr_dtvcc_process_data(void *dtvcc_rust, const unsigned char cc_valid,
    const unsigned char cc_type, const unsigned char data1, const unsigned char data2);
#endif
```

---

### Commit 6: `fce9fcfc` - Declare `ccxr_dtvcc_set_encoder` in C
**Status: NEEDED**

**Files:** `src/lib_ccx/lib_ccx.h`

**Key changes:**
```c
#ifndef DISABLE_RUST
void ccxr_dtvcc_set_encoder(void *dtvcc_rust, struct encoder_ctx* encoder);
#endif
```

---

### Commit 7: `e8a26608` - Use Rust init/free in C code
**Status: NEEDED**

Replace C dtvcc initialization with Rust version.

**Files:** `src/lib_ccx/ccx_decoders_common.c`

**Key changes:**
- In `init_cc_decode()`: Call `ccxr_dtvcc_init()` instead of `dtvcc_init()`
- In `dinit_cc_decode()`: Call `ccxr_dtvcc_free()` instead of `dtvcc_free()`
- Use `#ifndef DISABLE_RUST` guards

---

### Commit 8: `1ce5ad42` - Fix rust build
**Status: LIKELY OBSOLETE** - Will need fresh fixes for current codebase

---

### Commit 9: `cf2a335c` - Fix `is_active` value
**Status: NEEDED** (part of commit 1 implementation)

Ensures `dtvcc_rust.is_active` is set correctly from settings.

---

### Commit 10: `6cb6f6e3` - Change `ccxr_flush_decoder` to use `dtvcc_rust`
**Status: NEEDED**

**Files:**
- `src/rust/src/decoder/service_decoder.rs`
- `src/rust/src/decoder/mod.rs`
- `src/lib_ccx/ccx_decoders_common.h`

**Key changes:**
- Change `ccxr_flush_decoder` signature from raw pointers to Rust types
- Add `ccxr_flush_active_decoders` extern C function
- Remove old `ccxr_flush_decoder` extern declaration from C

---

### Commit 11: `e8cf6ae2` - Fix double free issue
**Status: NEEDED** (incorporated into memory management design)

---

### Commit 12-13: `3f2d2f2e`, `e1ca3a66` - Convert code portions to rust
**Status: NEEDED**

Changes `do_cb` to get `dtvcc` from `ctx.dtvcc_rust` instead of parameter.

**Files:** `src/rust/src/lib.rs`

**Key changes:**
```rust
pub fn do_cb(ctx: &mut lib_cc_decode, cc_block: &[u8]) -> bool {
    let dtvcc = unsafe { &mut *(ctx.dtvcc_rust as *mut Dtvcc) };
    // ...
}
```

Also modifies `ccxr_process_cc_data` to not create new Dtvcc.

---

### Commit 14: `f8160d76` - Revert formatting issues
**Status: OBSOLETE** - Was cleanup, not functionality

---

### Commit 15: `4c50f436` - Clippy fixes
**Status: OBSOLETE** - Will need fresh clippy fixes

---

### Commit 16: `7a7471c3` - Use Rust functions in mp4.c
**Status: NEEDED**

**Files:** `src/lib_ccx/mp4.c`

**Key changes:**
```c
#ifndef DISABLE_RUST
    ccxr_dtvcc_set_encoder(dec_ctx->dtvcc_rust, enc_ctx);
    ccxr_dtvcc_process_data(dec_ctx->dtvcc_rust, temp[0], temp[1], temp[2], temp[3]);
#else
    dec_ctx->dtvcc->encoder = (void *)enc_ctx;
    dtvcc_process_data(dec_ctx->dtvcc, (unsigned char *)temp);
#endif
```

---

### Commit 17: `609c4d03` - Fix mp4 arguments & clippy
**Status: OBSOLETE** - Bug fixes for commit 16, will be handled correctly

---

### Commit 18: `991de144` - Fix memory leaks
**Status: NEEDED** (incorporated into memory management design)

---

### Commit 19: `294b9d91` - Fix unit test errors
**Status: NEEDED** - Tests need to use new initialization

---

### Commit 20: `1ccaf033` - Merge master
**Status: OBSOLETE** - Was just a merge commit

---

## Implementation Steps

### Phase 1: Rust Core Changes ✅ COMPLETE

> **Implementation Note:** Instead of modifying the existing `Dtvcc` struct (which would break
> backward compatibility), we created a new `DtvccRust` struct. This allows the existing code
> to continue working while the new persistent context is available for Phase 2-3.

**Step 1.1: Add new `DtvccRust` struct** ✅
- File: `src/rust/src/decoder/mod.rs`
- Added `CCX_DTVCC_MAX_SERVICES` constant (63)
- Created new `DtvccRust` struct with:
  - `decoders: [Option<Box<dtvcc_service_decoder>>; CCX_DTVCC_MAX_SERVICES]` (owned)
  - `encoder: *mut encoder_ctx` (set later via `set_encoder()`)
  - Raw pointers for `report` and `timing` (owned by C)
- Implemented `DtvccRust::new(opts: &ccx_decoder_dtvcc_settings)`
- Used heap allocation (`alloc_zeroed`) to avoid stack overflow with large structs
- Added `set_encoder()`, `process_cc_data()`, `flush_active_decoders()` methods

**Step 1.2: Add FFI functions in lib.rs** ✅
- File: `src/rust/src/lib.rs`
- Added `ccxr_dtvcc_init()` - creates boxed DtvccRust, returns opaque `void*`
- Added `ccxr_dtvcc_free()` - properly frees all owned memory (decoders, tv_screens, rows)
- Added `ccxr_dtvcc_set_encoder()` - sets encoder pointer
- Added `ccxr_dtvcc_process_data()` - processes CC data on existing DtvccRust
- Added `ccxr_flush_active_decoders()` - flushes all active service decoders
- Added `ccxr_dtvcc_is_active()` - helper to check if context is active

**Step 1.3: Existing code unchanged** ✅
- The existing `Dtvcc` struct and `ccxr_process_cc_data()` remain unchanged
- This ensures backward compatibility during the transition
- Phase 2-3 will switch to using the new functions

**Step 1.4: Add unit tests** ✅
- Added 5 new tests for `DtvccRust`:
  - `test_dtvcc_rust_new` - verifies initialization
  - `test_dtvcc_rust_set_encoder` - verifies encoder setting
  - `test_dtvcc_rust_process_cc_data` - verifies CC data processing
  - `test_dtvcc_rust_clear_packet` - verifies packet clearing
  - `test_dtvcc_rust_state_persistence` - **key test** verifying state persists across calls
- All 176 tests pass (171 existing + 5 new)

### Phase 2: C Header Changes ✅ COMPLETE

**Step 2.1: Add `dtvcc_rust` to struct** ✅
- File: `src/lib_ccx/ccx_decoders_structs.h`
- Added `void *dtvcc_rust;` field to `lib_cc_decode` struct

**Step 2.2: Declare Rust FFI functions** ✅
- File: `src/lib_ccx/ccx_dtvcc.h`
- Added extern declarations for `ccxr_dtvcc_init`, `ccxr_dtvcc_free`, `ccxr_dtvcc_process_data`
- File: `src/lib_ccx/lib_ccx.h`
- Added extern declaration for `ccxr_dtvcc_set_encoder`
- File: `src/lib_ccx/ccx_decoders_common.h`
- Added extern declaration for `ccxr_flush_active_decoders`

### Phase 3: C Implementation Changes ✅ COMPLETE

**Step 3.1: Update decoder initialization/destruction** ✅
- File: `src/lib_ccx/ccx_decoders_common.c`
- In `init_cc_decode()`: Use `ccxr_dtvcc_init()` when Rust enabled
- In `dinit_cc_decode()`: Use `ccxr_dtvcc_free()` when Rust enabled
- In `flush_cc_decode()`: Use `ccxr_flush_active_decoders()` when Rust enabled
- Added `ccxr_dtvcc_is_active()` declaration to `ccx_dtvcc.h`

**Step 3.2: Update encoder assignment points** ✅
- File: `src/lib_ccx/general_loop.c`
- Three locations updated with `ccxr_dtvcc_set_encoder()` calls
- File: `src/lib_ccx/mp4.c`
- Updated with `ccxr_dtvcc_set_encoder()` and `ccxr_dtvcc_process_data()`

### Phase 4: Testing

**Step 4.1: Update Rust unit tests**
- File: `src/rust/src/lib.rs` (test module)
- File: `src/rust/src/decoder/mod.rs` (test module)
- Use new initialization pattern with `ccx_decoder_dtvcc_settings`

**Step 4.2: Build verification**
- Verify build with Rust enabled
- Verify build with `DISABLE_RUST`
- Run existing test suite

**Step 4.3: Functional testing**
- Test CEA-708 decoding with MP4 files
- Test CEA-708 decoding with MPEG-TS files
- Verify state persistence across multiple CC data blocks

---

## Risk Areas

1. **Memory management**: The Rust `Dtvcc` owns `Box`ed decoders and TV screens that must be properly freed
2. **Lifetime issues**: The `'a` lifetime on `Dtvcc` references `timing` and `report` - ensure these outlive the Dtvcc
3. **Thread safety**: Raw pointers are used; ensure single-threaded access
4. **Bindgen compatibility**: Ensure `lib_cc_decode` bindings include the new `dtvcc_rust` field

---

## Files Modified Summary

### Phase 1 (Complete)

| File | Type | Status | Changes |
|------|------|--------|---------|
| `src/rust/src/decoder/mod.rs` | Rust | ✅ | Added `DtvccRust` struct (+260 lines), tests (+120 lines) |
| `src/rust/src/lib.rs` | Rust | ✅ | Added 6 FFI functions (+130 lines) |

### Phase 2 (Complete)

| File | Type | Status | Changes |
|------|------|--------|---------|
| `src/lib_ccx/ccx_decoders_structs.h` | C Header | ✅ | Added `dtvcc_rust` field |
| `src/lib_ccx/ccx_dtvcc.h` | C Header | ✅ | Added extern declarations for init/free/process_data |
| `src/lib_ccx/lib_ccx.h` | C Header | ✅ | Added `ccxr_dtvcc_set_encoder` declaration |
| `src/lib_ccx/ccx_decoders_common.h` | C Header | ✅ | Added `ccxr_flush_active_decoders` declaration |

### Phase 3 (Complete)

| File | Type | Status | Changes |
|------|------|--------|---------|
| `src/lib_ccx/ccx_decoders_common.c` | C | ✅ | Use Rust init/free/flush with `#ifndef DISABLE_RUST` guards |
| `src/lib_ccx/general_loop.c` | C | ✅ | Set encoder via `ccxr_dtvcc_set_encoder()` at 3 locations |
| `src/lib_ccx/mp4.c` | C | ✅ | Use `ccxr_dtvcc_set_encoder()` and `ccxr_dtvcc_process_data()` |
| `src/lib_ccx/ccx_dtvcc.h` | C Header | ✅ | Added `ccxr_dtvcc_is_active()` declaration |

---

## Estimated Complexity

- **Phase 1 (Rust)**: ✅ COMPLETE - Added new struct alongside existing code
- **Phase 2 (C Headers)**: ✅ COMPLETE - Added field and extern declarations
- **Phase 3 (C Implementation)**: ✅ COMPLETE - Added conditional compilation blocks
- **Phase 4 (Testing)**: Medium - need to verify state persistence works correctly

**Total estimate**: This is a significant change touching core decoder logic. Recommend implementing in small, testable increments.

---

## Next Steps

1. Create PR for Phase 1 (pending CI validation)
2. After Phase 1 PR is merged, implement Phase 2 (C Headers)
3. After Phase 2 PR is merged, implement Phase 3 (C Implementation)
4. Full integration testing in Phase 4
