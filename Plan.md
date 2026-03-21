# JPEG Encoder Implementation Plan (Jpeg32 library)

## Project Overview

Implement baseline JPEG encoder per Design.md specification. Target: AT32F437 Cortex-M4F with cross-platform Windows testing support.

---

## Phase 1: Project Setup

### 1.1 Directory Structure (under project root)
- [x] Create `include/` directory
- [x] Create `src/` directory
- [x] Create `test/` directory
- [x] Create `test/test_images/` directory
- [x] Create `tools/` directory
- [x] Create `cmake/` directory
- [x] Create `plans/` directory

### 1.2 Build System
- [x] Create `CMakeLists.txt` (root)
- [x] Create `cmake/arm-none-eabi.cmake` toolchain file
- [x] Configure cross-platform detection (ARM vs Windows)
- [x] Add test targets configuration
- [x] Add install rules

### 1.3 Documentation
- [x] Copy `Design.md` to project root
- [x] Create `README.md` with build instructions

---

## Phase 2: Header Files

### 2.1 jpeg_config.h
- [x] Define `JPEG_QUALITY_DEFAULT`, `JPEG_QUALITY_MIN`, `JPEG_QUALITY_MAX`
- [x] Define `JPEG_BLOCK_SIZE` (8)
- [x] Define MCU size constants
- [x] Define platform detection macros (`JPEG_USE_ARM_DSP` using `__ARM_FEATURE_DSP`)
- [x] Define `JPEG_DEFAULT_BLOCK_COPY` macro
- [x] Add debug/trace macros

### 2.2 jpeg_encoder.h
- [x] Define `jpeg_format_t` enum (RGB888, RGB565, YUYV, YUV420, GRAYSCALE)
- [x] Define `jpeg_subsample_t` enum (444, 420, 422, GRAY)
- [x] Define `jpeg_status_t` enum (OK, error codes)
- [x] Define `jpeg_block_copy_fn` function pointer type with signature:
  ```c
  typedef void (*jpeg_block_copy_fn)(
      const uint8_t *src, int16_t *dst_block,
      uint32_t src_stride, uint16_t x, uint16_t y,
      uint16_t img_w, uint16_t img_h
  );
  ```
- [x] Define `jpeg_config_t` structure
- [x] Define `jpeg_encoder_t` structure with:
  - `quant_recip_y[64]`, `quant_recip_c[64]` (reciprocals, not tables)
  - `current_block_in_mcu` for streaming support
  - `block[64]` for in-place DCT (no separate dct_workspace)
  - `bit_buffer` as `uint32_t` (accumulates up to 24 bits)
- [x] Declare public API functions
- [x] Add include guards

### 2.3 jpeg_tables.h
- [x] Declare `jpeg_quant_table_y[64]` (luminance, base values)
- [x] Declare `jpeg_quant_table_c[64]` (chrominance, base values)
- [x] Declare Huffman DC luminance tables (bits, vals)
- [x] Declare Huffman DC chrominance tables (bits, vals)
- [x] Declare Huffman AC luminance tables (bits, vals)
- [x] Declare Huffman AC chrominance tables (bits, vals)
- [x] Declare `jpeg_zigzag[64]` scan order
- [x] Define `huff_entry_t` structure for O(1) Huffman lookup:
  ```c
  typedef struct {
      uint8_t  symbol;
      uint8_t  code_len;
      uint16_t code;
  } huff_entry_t;
  ```
- [x] Declare pre-decoded Huffman lookup tables (`huff_dc_y[12]`, `huff_ac_y[256]`, etc.)
- [x] Define color conversion constants (fixed-point)

### 2.4 jpeg_block_copy.h
- [x] Declare `jpeg_block_copy_simple()` with signature:
  ```c
  void jpeg_block_copy_simple(const uint8_t *src, int16_t *dst_block,
      uint32_t src_stride, uint16_t x, uint16_t y,
      uint16_t img_w, uint16_t img_h);
  ```
- [x] Implement edge replication for boundary blocks
- [x] Apply level-shift (-128) to output
- [x] Declare `jpeg_block_copy_edma()` (stub for ARM)
- [x] Declare `jpeg_block_copy_edma_init()`
- [x] Declare `jpeg_block_copy_edma_is_busy()`
- [x] Add include guards

---

## Phase 3: Table Data

### 3.1 jpeg_tables.c
- [x] Define standard luminance quantization table (ITU-R BT.601) - base values
- [x] Define standard chrominance quantization table - base values
- [x] Define Huffman DC luminance table (bits count array)
- [x] Define Huffman DC luminance table (values array)
- [x] Define Huffman DC chrominance table (bits count array)
- [x] Define Huffman DC chrominance table (values array)
- [x] Define Huffman AC luminance table (bits count array)
- [x] Define Huffman AC luminance table (values array)
- [x] Define Huffman AC chrominance table (bits count array)
- [x] Define Huffman AC chrominance table (values array)
- [x] Define zigzag scan order array
- [x] Implement `jpeg_build_huffman_lookup()` to decode Huffman tables into `huff_entry_t` arrays
- [x] Pre-compute Huffman lookup tables on init

### 3.2 jpeg_block_copy.c
- [x] Implement `jpeg_block_copy_simple()` with row-by-row memcpy
- [x] Handle source stride correctly
- [x] Handle edge blocks (partial blocks with padding)

---

## Phase 4: Core Processing Modules

### 4.1 jpeg_block_copy.c
- [x] Implement `jpeg_block_copy_simple()` with row-by-row memcpy
- [x] Handle source stride correctly
- [x] Handle edge blocks (partial blocks with padding)
- [x] Implement `jpeg_block_copy_edma_init()` (stub or ARM-only)
- [x] Implement `jpeg_block_copy_edma()` (stub or ARM-only)
- [x] Implement `jpeg_block_copy_edma_is_busy()` (stub or ARM-only)

### 4.1 jpeg_block_copy.c
- [x] Implement `jpeg_block_copy_simple()` with row-by-row memcpy
- [x] Handle source stride correctly
- [x] Handle edge blocks with pixel replication (not zero-fill)
- [x] Apply level-shift (-128) during copy
- [x] Output to `int16_t` block (not `uint8_t`)
- [x] Implement `jpeg_block_copy_edma_init()` (stub or ARM-only)
- [x] Implement `jpeg_block_copy_edma()` (stub or ARM-only)
- [x] Implement `jpeg_block_copy_edma_is_busy()` (stub or ARM-only)

### 4.2 jpeg_color.c
- [x] Implement `jpeg_rgb_to_ycbcr()` for single pixel (fixed-point)
- [x] Implement `jpeg_rgb888_to_ycbcr_block()` for 8×8 block
- [x] Implement `jpeg_rgb565_to_ycbcr_block()` for 8×8 block
- [x] Implement `jpeg_yuyv_to_ycbcr_block()` for 8×8 block
- [x] Implement `jpeg_yuv420_to_ycbcr_block()` for 8×8 block
- [x] Implement `jpeg_ycbcr_subsample_420()` for chroma subsampling
- [x] Implement `jpeg_ycbcr_subsample_422()` for chroma subsampling
- [x] Add SIMD optimization guards for ARM

### 4.3 jpeg_dct.c
- [x] Implement `jpeg_dct8x8_ref()` reference C implementation (in-place, 32-bit intermediates)
- [x] Implement `jpeg_dct8x8_arm()` using `__SSAT` and `__SMLAD` intrinsics (AAN algorithm)
- [x] Implement `jpeg_dct8x8()` with platform selection via `#if JPEG_USE_ARM_DSP`
- [x] DCT operates in-place on `block[64]` (no separate output buffer)
- [x] Use 16-bit coefficients with 32-bit accumulators for AAN algorithm
- [x] Verify ARM output matches reference within tolerance

### 4.4 jpeg_quantize.c
- [x] Implement `jpeg_quantize_scale()` to scale base tables by quality
- [x] Implement `jpeg_init_quant_tables()` to compute reciprocals:
  ```c
  recip[k] = (65536 + quant[k]/2) / quant[k]
  ```
- [x] Implement `jpeg_quantize_block()` using reciprocal multiplication:
  ```c
  coeff = (block[i] * recip[i]) >> 16
  ```
- [x] Implement `jpeg_zigzag_reorder()` to reorder coefficients
- [x] Implement combined `jpeg_quantize_zigzag()` for efficiency

### 4.5 jpeg_huffman.c
- [x] Implement `jpeg_huff_encode_dc()` using O(1) huff_entry lookup
- [x] Implement `jpeg_huff_encode_ac()` using O(1) lookup with (run<<4)|category index
- [x] Implement `jpeg_bitstream_write()` using 32-bit `bit_buffer`
- [x] Implement `jpeg_bitstream_flush()` to flush remaining bits
- [x] Handle 0xFF byte stuffing (0xFF → 0xFF 0x00)
- [x] Use 32-bit bit_buffer accumulating up to 24 bits before flush

---

## Phase 5: Header Construction

### 5.1 jpeg_header.c
- [x] Implement `jpeg_write_soi()` - Start of Image marker
- [x] Implement `jpeg_write_app0()` - JFIF marker segment
- [x] Implement `jpeg_write_dqt()` - Quantization tables
- [x] Implement `jpeg_write_sof0()` - Start of Frame (Baseline)
- [x] Implement `jpeg_write_dht()` - Huffman tables
- [x] Implement `jpeg_write_sos()` - Start of Scan
- [x] Implement `jpeg_write_eoi()` - End of Image marker
- [x] Implement `jpeg_build_header()` - Assemble all header markers
- [x] Calculate correct segment lengths

---

## Phase 6: Main Encoder

### 6.1 jpeg_encoder.c
- [x] Implement `jpeg_encoder_init()` - Initialize state and config
- [x] Calculate MCU dimensions from subsampling mode
- [x] Call `jpeg_init_quant_tables()` to compute reciprocals from quality
- [x] Initialize DC predictors to zero
- [x] Implement `jpeg_encoder_encode()` - Full image blocking encode
- [x] Implement `jpeg_encoder_encode_mcu()` - Single MCU streaming encode
- [x] Track `current_block_in_mcu` for multi-block MCU resumption (4:2:0 = 6 blocks)
- [x] Implement MCU coordinate calculation from `current_mcu` index
- [x] Implement block extraction from input buffer using pluggable `block_copy`
- [x] Chain processing steps: copy (+level-shift) → color → DCT (in-place) → quantize+zigzag → Huffman
- [x] Update DC predictors after each block
- [x] Implement `jpeg_encoder_get_output_size()`
- [x] Implement `jpeg_encoder_reset()`
- [x] Handle edge MCUs with pixel replication

---

## Phase 7: Unit Tests

### 7.1 Test Framework Setup
- [ ] Create `test/test_main.c` with test runner
- [ ] Create simple test assertion macros
- [ ] Create `test/test_unit.c` for unit tests

### 7.2 Color Conversion Tests
- [ ] Test RGB888 to YCbCr accuracy (known values)
- [ ] Test RGB565 to YCbCr accuracy
- [ ] Test boundary values (0, 128, 255)
- [ ] Test subsampling 4:2:0 correctness
- [ ] Test subsampling 4:2:2 correctness

### 7.3 DCT Tests
- [ ] Test DCT of constant block (should have DC only)
- [ ] Test DCT of impulse (single non-zero pixel)
- [ ] Test DCT forward and inverse consistency
- [ ] Test fixed-point precision with known values
- [ ] Compare ARM DSP intrinsics vs reference output

### 7.4 Quantization Tests
- [ ] Test zigzag reorder correctness
- [ ] Test reciprocal computation: `recip = (65536 + q/2) / q`
- [ ] Test reciprocal quantization: `(coeff * recip) >> 16`
- [ ] Test quantization with quality factor scaling
- [ ] Test quantization of known coefficient block

### 7.5 Huffman Tests
- [ ] Test DC Huffman encoding of known values
- [ ] Test AC Huffman encoding with run-length
- [ ] Test bitstream output correctness
- [ ] Test byte stuffing for 0xFF

### 7.6 Block Copy Tests
- [ ] Test simple copy with stride
- [ ] Test partial block at image edge with pixel replication
- [ ] Test level-shift (-128) application
- [ ] Test output to int16_t block

### 7.7 Header Tests
- [ ] Test SOI marker output
- [ ] Test APP0 segment format
- [ ] Test DQT segment with both tables
- [ ] Test SOF0 segment for each subsampling mode
- [ ] Test DHT segment format
- [ ] Test SOS segment format
- [ ] Verify header byte-by-byte against expected

---

## Phase 8: End-to-End Tests

### 8.1 Test Infrastructure
- [ ] Create `test/test_e2e.c`
- [ ] Create test image generator (solid colors)
- [ ] Create test image generator (gradients)
- [ ] Create test image generator (patterns)
- [ ] Save encoded JPEG to file

### 8.2 Verification Script
- [ ] Create `tools/verify_jpeg.py`
- [ ] Implement JPEG file validation (can be opened)
- [ ] Implement pixel comparison with tolerance
- [ ] Add command-line interface
- [ ] Add exit codes for CI integration

### 8.3 E2E Test Cases
- [ ] Test 8×8 solid white image
- [ ] Test 8×8 solid black image
- [ ] Test 8×8 solid gray (128) image
- [ ] Test 16×16 two-color pattern
- [ ] Test 64×64 gradient image
- [ ] Test 640×480 test pattern
- [ ] Test grayscale mode encoding
- [ ] Test 4:2:0 subsampling
- [ ] Test 4:2:2 subsampling
- [ ] Test 4:4:4 subsampling
- [ ] Test quality factor variations (10, 50, 90)
- [ ] Test edge case: width not multiple of MCU
- [ ] Test edge case: height not multiple of MCU

---

## Phase 9: ARM Platform Integration

### 9.1 ARM DSP Intrinsics Integration
- [x] Verify `__ARM_FEATURE_DSP` is defined for AT32F437
- [x] Use `__SSAT` for saturating arithmetic in DCT
- [x] Use `__SMLAD` for multiply-accumulate in AAN DCT
- [x] Implement `jpeg_dct8x8_arm()` using intrinsics
- [ ] Test on ARM hardware or simulator

### 9.2 EDMA Block Copy (Optional)
- [ ] Include AT32 HAL headers
- [ ] Implement EDMA channel configuration
- [ ] Implement `jpeg_block_copy_edma()`
- [ ] Implement linked-list mode for 2D copy
- [ ] Add DMA completion interrupt handler
- [ ] Test DMA copy correctness

---

## Phase 10: Optimization

### 10.1 Fixed-Point DCT Optimization
- [x] Optimize AAN DCT with loop unrolling
- [x] Use inline functions for frequently called operations
- [x] Profile DCT performance target: 8 multiplies, 26 additions per 1D DCT

### 10.2 Quantization Optimization
- [x] Unroll quantization loop over 64 coefficients
- [x] Use reciprocal multiplication (already faster than division)

### 10.3 Huffman Encoding Optimization
- [x] Use 32-bit bit_buffer (reduces stores 3×)
- [x] Pre-compute Huffman lookup tables for O(1) access

### 10.4 Memory Optimization
- [x] Use `__attribute__((aligned(4)))` for buffers
- [x] Minimize stack usage in functions
- [x] Profile and optimize hot paths

---

## Phase 11: Documentation and Finalization

### 11.1 Code Documentation
- [x] Add file headers to all source files
- [x] Add function documentation (Doxygen style)
- [x] Document all public APIs
- [x] Document configuration options

### 11.2 README
- [x] Write project description
- [x] Write build instructions (Windows)
- [x] Write build instructions (ARM)
- [x] Write usage example
- [x] Write test instructions
- [x] Add license information

### 11.3 Final Verification
- [ ] Run all unit tests
- [ ] Run all E2E tests
- [ ] Verify on Windows platform
- [ ] Verify on ARM platform (if available)
- [ ] Run `ctest --output-on-failure`
- [ ] Check memory usage
- [ ] Check output file sizes

---

## Verification Checklist

### Before Commit
- [x] All files have proper license headers
- [x] No compiler warnings
- [ ] All tests pass
- [x] Code formatted consistently
- [x] No hardcoded paths or credentials

### Release Criteria
- [ ] Unit tests: 100% pass rate
- [ ] E2E tests: All JPEGs valid and decodable
- [ ] Performance: 5MP encoding < 1 second
- [ ] Memory: < 1MB working set (excluding buffers)
- [ ] Cross-platform: Builds on Windows and ARM

### Memory Requirements (per Design.md)

| Buffer | Size |
|--------|------|
| Encoder workspace | 384 bytes |
| Quantization reciprocals | 256 bytes |
| Huffman lookup tables | ~550 bytes |

---

## Estimated Effort

| Phase | Estimated Hours |
|-------|-----------------|
| Phase 1: Project Setup | 2 |
| Phase 2: Header Files | 3 |
| Phase 3: Table Data | 2 |
| Phase 4: Core Modules | 12 |
| Phase 5: Header Construction | 4 |
| Phase 6: Main Encoder | 6 |
| Phase 7: Unit Tests | 8 |
| Phase 8: E2E Tests | 6 |
| Phase 9: ARM Integration | 2 |
| Phase 10: Optimization | 4 |
| Phase 11: Documentation | 3 |
| **Total** | **52 hours** |

---

## Dependencies

| Dependency | Phase | Notes |
|------------|-------|-------|
| CMake | Phase 1 | Build system |
| ARM DSP Intrinsics | Phase 9 | `__SSAT`, `__SMLAD` for AAN DCT |
| Python 3 + PIL (via uv) | Phase 8 | Test verification |
| ARM toolchain | Phase 9 | Cross-compilation |
| AT32 SDK | Phase 9 | EDMA support |

---

## Risk Items

| Risk | Mitigation |
|------|------------|
| DCT precision issues | Use 16-bit fixed-point AAN with 32-bit accumulators, verify precision |
| Huffman edge cases | Extensive testing with boundary values |
| Memory alignment on ARM | Use aligned attributes, test on hardware |
| Byte stuffing bugs | Unit test with 0xFF patterns |
| Endianness issues | Test on both little-endian platforms |
