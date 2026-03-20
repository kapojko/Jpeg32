# JPEG Encoder Implementation Plan (Jpeg32 library)

## Project Overview

Implement baseline JPEG encoder per Design.md specification. Target: AT32F437 Cortex-M4F with cross-platform Windows testing support.

---

## Phase 1: Project Setup

### 1.1 Directory Structure (under project root)
- [ ] Create `include/` directory
- [ ] Create `src/` directory
- [ ] Create `test/` directory
- [ ] Create `test/test_images/` directory
- [ ] Create `tools/` directory
- [ ] Create `cmake/` directory
- [ ] Create `plans/` directory

### 1.2 Build System
- [ ] Create `CMakeLists.txt` (root)
- [ ] Create `cmake/arm-none-eabi.cmake` toolchain file
- [ ] Configure cross-platform detection (ARM vs Windows)
- [ ] Add test targets configuration
- [ ] Add install rules

### 1.3 Documentation
- [ ] Copy `Design.md` to project root
- [ ] Create `README.md` with build instructions

---

## Phase 2: Header Files

### 2.1 jpeg_config.h
- [ ] Define `JPEG_QUALITY_DEFAULT`, `JPEG_QUALITY_MIN`, `JPEG_QUALITY_MAX`
- [ ] Define `JPEG_BLOCK_SIZE` (8)
- [ ] Define MCU size constants
- [ ] Define platform detection macros (`JPEG_USE_CMSIS_DSP`)
- [ ] Define `JPEG_DEFAULT_BLOCK_COPY` macro
- [ ] Add debug/trace macros

### 2.2 jpeg_encoder.h
- [ ] Define `jpeg_format_t` enum (RGB888, RGB565, YUYV, YUV420, GRAYSCALE)
- [ ] Define `jpeg_subsample_t` enum (444, 420, 422, GRAY)
- [ ] Define `jpeg_status_t` enum (OK, error codes)
- [ ] Define `jpeg_block_copy_fn` function pointer type
- [ ] Define `jpeg_config_t` structure
- [ ] Define `jpeg_encoder_t` structure
- [ ] Declare public API functions
- [ ] Add include guards

### 2.3 jpeg_tables.h
- [ ] Declare `jpeg_quant_table_y[64]` (luminance)
- [ ] Declare `jpeg_quant_table_c[64]` (chrominance)
- [ ] Declare Huffman DC luminance tables (bits, vals)
- [ ] Declare Huffman DC chrominance tables (bits, vals)
- [ ] Declare Huffman AC luminance tables (bits, vals)
- [ ] Declare Huffman AC chrominance tables (bits, vals)
- [ ] Declare `jpeg_zigzag[64]` scan order
- [ ] Define color conversion constants (fixed-point)

### 2.4 jpeg_block_copy.h
- [ ] Declare `jpeg_block_copy_simple()`
- [ ] Declare `jpeg_block_copy_edma()` (stub for ARM)
- [ ] Declare `jpeg_block_copy_edma_init()`
- [ ] Declare `jpeg_block_copy_edma_is_busy()`
- [ ] Add include guards

---

## Phase 3: Table Data

### 3.1 jpeg_tables.c
- [ ] Define standard luminance quantization table (ITU-R BT.601)
- [ ] Define standard chrominance quantization table
- [ ] Define Huffman DC luminance table (bits count array)
- [ ] Define Huffman DC luminance table (values array)
- [ ] Define Huffman DC chrominance table (bits count array)
- [ ] Define Huffman DC chrominance table (values array)
- [ ] Define Huffman AC luminance table (bits count array)
- [ ] Define Huffman AC luminance table (values array)
- [ ] Define Huffman AC chrominance table (bits count array)
- [ ] Define Huffman AC chrominance table (values array)
- [ ] Define zigzag scan order array

### 3.2 jpeg_block_copy.c
- [ ] Implement `jpeg_block_copy_simple()` with row-by-row memcpy
- [ ] Handle source stride correctly
- [ ] Handle edge blocks (partial blocks with padding)

---

## Phase 4: Core Processing Modules

### 4.1 jpeg_block_copy.c
- [ ] Implement `jpeg_block_copy_simple()` with row-by-row memcpy
- [ ] Handle source stride correctly
- [ ] Handle edge blocks (partial blocks with padding)
- [ ] Implement `jpeg_block_copy_edma_init()` (stub or ARM-only)
- [ ] Implement `jpeg_block_copy_edma()` (stub or ARM-only)
- [ ] Implement `jpeg_block_copy_edma_is_busy()` (stub or ARM-only)

### 4.2 jpeg_color.c
- [ ] Implement `jpeg_rgb_to_ycbcr()` for single pixel (fixed-point)
- [ ] Implement `jpeg_rgb888_to_ycbcr_block()` for 8×8 block
- [ ] Implement `jpeg_rgb565_to_ycbcr_block()` for 8×8 block
- [ ] Implement `jpeg_yuyv_to_ycbcr_block()` for 8×8 block
- [ ] Implement `jpeg_yuv420_to_ycbcr_block()` for 8×8 block
- [ ] Implement `jpeg_ycbcr_subsample_420()` for chroma subsampling
- [ ] Implement `jpeg_ycbcr_subsample_422()` for chroma subsampling
- [ ] Add SIMD optimization guards for ARM

### 4.3 jpeg_dct.c
- [ ] Implement `jpeg_dct8x8_ref()` reference C implementation
- [ ] Implement `jpeg_dct8x8_init()` for CMSIS-DSP initialization
- [ ] Implement `jpeg_dct8x8()` with platform selection
- [ ] Add `#if JPEG_USE_CMSIS_DSP` branch for CMSIS-DSP
- [ ] Verify output matches reference within tolerance

### 4.4 jpeg_quantize.c
- [ ] Implement `jpeg_quantize_scale()` to scale tables by quality
- [ ] Implement `jpeg_quantize_block()` divide and round
- [ ] Implement `jpeg_zigzag_reorder()` to reorder coefficients
- [ ] Implement combined `jpeg_quantize_zigzag()` for efficiency

### 4.5 jpeg_huffman.c
- [ ] Implement `jpeg_huff_encode_dc()` for DC coefficient
- [ ] Implement `jpeg_huff_encode_ac()` for AC coefficients with RLE
- [ ] Implement `jpeg_bitstream_write()` for bit-level output
- [ ] Implement `jpeg_bitstream_flush()` to flush remaining bits
- [ ] Handle byte stuffing for 0xFF bytes in output

---

## Phase 5: Header Construction

### 5.1 jpeg_header.c
- [ ] Implement `jpeg_write_soi()` - Start of Image marker
- [ ] Implement `jpeg_write_app0()` - JFIF marker segment
- [ ] Implement `jpeg_write_dqt()` - Quantization tables
- [ ] Implement `jpeg_write_sof0()` - Start of Frame (Baseline)
- [ ] Implement `jpeg_write_dht()` - Huffman tables
- [ ] Implement `jpeg_write_sos()` - Start of Scan
- [ ] Implement `jpeg_write_eoi()` - End of Image marker
- [ ] Implement `jpeg_build_header()` - Assemble all header markers
- [ ] Calculate correct segment lengths

---

## Phase 6: Main Encoder

### 6.1 jpeg_encoder.c
- [ ] Implement `jpeg_encoder_init()` - Initialize state and config
- [ ] Calculate MCU dimensions from subsampling mode
- [ ] Scale quantization tables based on quality factor
- [ ] Initialize DC predictors to zero
- [ ] Implement `jpeg_encoder_encode()` - Full image blocking encode
- [ ] Implement `jpeg_encoder_encode_mcu()` - Single MCU streaming encode
- [ ] Implement MCU coordinate calculation
- [ ] Implement block extraction from input buffer
- [ ] Chain processing steps: copy → color → DCT → quantize → Huffman
- [ ] Update DC predictors after each block
- [ ] Implement `jpeg_encoder_get_output_size()`
- [ ] Implement `jpeg_encoder_reset()`
- [ ] Handle edge MCUs (image not multiple of MCU size)

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
- [ ] Compare CMSIS-DSP vs reference output

### 7.4 Quantization Tests
- [ ] Test zigzag reorder correctness
- [ ] Test quantization with quality factor scaling
- [ ] Test quantization of known coefficient block

### 7.5 Huffman Tests
- [ ] Test DC Huffman encoding of known values
- [ ] Test AC Huffman encoding with run-length
- [ ] Test bitstream output correctness
- [ ] Test byte stuffing for 0xFF

### 7.6 Block Copy Tests
- [ ] Test simple copy with stride
- [ ] Test partial block at image edge
- [ ] Test various block sizes

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

### 9.1 CMSIS-DSP Integration
- [ ] Add CMSIS-DSP include path
- [ ] Link `arm_cortexM4lf_math.lib`
- [ ] Implement DCT using `arm_dct4_f32()`
- [ ] Initialize DCT instance in encoder init
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

### 10.1 SIMD Color Conversion
- [ ] Implement RGB→YCbCr using ARM SIMD intrinsics
- [ ] Use `__SADD8`, `__QADD8` for parallel operations
- [ ] Benchmark vs non-SIMD version

### 10.2 Loop Unrolling
- [ ] Unroll 8×8 DCT block loops
- [ ] Unroll zigzag reorder loop
- [ ] Unroll quantization loop

### 10.3 Memory Optimization
- [ ] Use `__attribute__((aligned(4)))` for buffers
- [ ] Minimize stack usage in functions
- [ ] Profile and optimize hot paths

---

## Phase 11: Documentation and Finalization

### 11.1 Code Documentation
- [ ] Add file headers to all source files
- [ ] Add function documentation (Doxygen style)
- [ ] Document all public APIs
- [ ] Document configuration options

### 11.2 README
- [ ] Write project description
- [ ] Write build instructions (Windows)
- [ ] Write build instructions (ARM)
- [ ] Write usage example
- [ ] Write test instructions
- [ ] Add license information

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
- [ ] All files have proper license headers
- [ ] No compiler warnings
- [ ] All tests pass
- [ ] Code formatted consistently
- [ ] No hardcoded paths or credentials

### Release Criteria
- [ ] Unit tests: 100% pass rate
- [ ] E2E tests: All JPEGs valid and decodable
- [ ] Performance: 5MP encoding < 1 second
- [ ] Memory: < 1MB working set (excluding buffers)
- [ ] Cross-platform: Builds on Windows and ARM

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
| Phase 9: ARM Integration | 4 |
| Phase 10: Optimization | 6 |
| Phase 11: Documentation | 3 |
| **Total** | **56 hours** |

---

## Dependencies

| Dependency | Phase | Notes |
|------------|-------|-------|
| CMake | Phase 1 | Build system |
| CMSIS-DSP | Phase 9 | ARM platform only |
| Python 3 + PIL (via uv) | Phase 8 | Test verification |
| ARM toolchain | Phase 9 | Cross-compilation |
| AT32 SDK | Phase 9 | EDMA support |

---

## Risk Items

| Risk | Mitigation |
|------|------------|
| DCT precision issues | Use 32-bit fixed-point or double-check float precision |
| Huffman edge cases | Extensive testing with boundary values |
| Memory alignment on ARM | Use aligned attributes, test on hardware |
| Byte stuffing bugs | Unit test with 0xFF patterns |
| Endianness issues | Test on both little-endian platforms |
