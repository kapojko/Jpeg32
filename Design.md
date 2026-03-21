# JPEG Encoder Design Document (Jpeg32 library)

## Overview

Baseline JPEG encoder for embedded systems (target: AT32F437 Cortex-M4F @ 288MHz). Features pluggable block copy for DMA optimization and cross-platform testing support.

---

## Architecture

### JPEG Structure

```
┌─────────────────────────────────────────────────────┐
│  HEADER                                              │
│  SOI → APP0 → DQT → SOF0 → DHT → SOS                │
├─────────────────────────────────────────────────────┤
│  ENTROPY-CODED DATA                                  │
│  [MCU_1][MCU_2][MCU_3]...[MCU_N]                    │
├─────────────────────────────────────────────────────┤
│  TAIL                                                │
│  EOI                                                 │
└─────────────────────────────────────────────────────┘
```

### MCU (Minimum Coded Unit)

| Subsampling | MCU Size | DCT Blocks | Block Order |
|-------------|----------|------------|-------------|
| 4:4:4 | 8×8 | 3 | Y, Cb, Cr |
| 4:2:2 | 16×8 | 4 | Y1, Y2, Cb, Cr |
| 4:2:0 | 16×16 | 6 | Y1, Y2, Y3, Y4, Cb, Cr |
| Gray | 8×8 | 1 | Y |

### Processing Order

- MCU order: Left → Right, Top → Bottom
- Within MCU: Sequential block order (see table above)
- DC encoding: Differential (current - previous)
- AC encoding: Independent
- DC predictors: Initialized to 0 at start of image
- Edge MCUs: Padded with replication of edge pixels (not zero-filled)

---

## File Structure

```
Jpeg32/                         # Top-level project directory
├── include/
│   ├── jpeg_encoder.h          # Main API
│   ├── jpeg_config.h           # Compile configuration
│   ├── jpeg_tables.h           # Quantization/Huffman tables
│   └── jpeg_block_copy.h       # Block copy interface
├── src/
│   ├── jpeg_encoder.c          # Main orchestrator
│   ├── jpeg_header.c           # Header construction
│   ├── jpeg_color.c            # Color conversion
│   ├── jpeg_dct.c              # DCT transform
│   ├── jpeg_quantize.c         # Quantization
│   ├── jpeg_huffman.c          # Huffman encoding
│   ├── jpeg_tables.c           # Table data
│   └── jpeg_block_copy.c       # Block copy implementations
├── test/
│   ├── test_main.c             # Test runner
│   ├── test_unit.c             # Unit tests
│   ├── test_e2e.c              # End-to-end tests
│   └── test_images/            # Test images (raw RGB)
├── tools/
│   └── verify_jpeg.py          # JPEG verification script
├── CMakeLists.txt
└── Design.md
```

---

## Data Structures

### Configuration

```c
typedef enum {
    JPEG_FMT_RGB888,
    JPEG_FMT_RGB565,
    JPEG_FMT_YUYV,
    JPEG_FMT_YUV420,
    JPEG_FMT_GRAYSCALE
} jpeg_format_t;

typedef enum {
    JPEG_SUBSAMPLE_444,
    JPEG_SUBSAMPLE_420,
    JPEG_SUBSAMPLE_422,
    JPEG_SUBSAMPLE_GRAY
} jpeg_subsample_t;

typedef void (*jpeg_block_copy_fn)(
    const uint8_t *src, int16_t *dst_block,
    uint32_t src_stride, uint16_t x, uint16_t y,
    uint16_t img_w, uint16_t img_h
);

typedef enum {
    JPEG_STATUS_OK = 0,
    JPEG_STATUS_ERROR_INVALID_PARAM = -1,
    JPEG_STATUS_ERROR_BUFFER_TOO_SMALL = -2,
    JPEG_STATUS_ERROR_UNSUPPORTED_FORMAT = -3,
    JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE = -4,
    JPEG_STATUS_ERROR_QUALITY_OUT_OF_RANGE = -5
} jpeg_status_t;

typedef struct {
    uint16_t            width;
    uint16_t            height;
    jpeg_format_t       input_format;
    jpeg_subsample_t    subsample_mode;
    uint8_t             quality;            // 1-100
    const uint8_t      *input_buffer;
    uint8_t            *output_buffer;
    uint32_t            output_buffer_size;
    uint32_t            input_stride;
    jpeg_block_copy_fn  block_copy;
} jpeg_config_t;
```

### Encoder State

```c
typedef struct {
    jpeg_config_t       config;
    
    // Derived parameters
    uint16_t            mcu_width;
    uint16_t            mcu_height;
    uint16_t            mcu_count_x;
    uint16_t            mcu_count_y;
    uint32_t            total_mcus;
    
    // Scaled quantization reciprocals (multiplier-based division)
    // recip[k] = (65536 + quant[k]/2) / quant[k]
    uint16_t            quant_recip_y[64];
    uint16_t            quant_recip_c[64];
    
    // DC predictors (persist across MCUs)
    int16_t             prev_dc_y;
    int16_t             prev_dc_cb;
    int16_t             prev_dc_cr;
    
    // Runtime state
    uint32_t            current_mcu;
    uint8_t             current_block_in_mcu;  // 0-5 for 4:2:0 tracking
    uint32_t            output_bytes;
    
    // Workspaces
    int16_t             block[64];              // In-place DCT input/output
    
    // Huffman state (32-bit buffer for efficiency)
    uint32_t            bit_buffer;             // Accumulates up to 24 bits
    uint8_t             bits_in_buffer;         // 0-23, flush when >= 16
} jpeg_encoder_t;
```

---

## API

```c
// Initialize encoder with configuration
jpeg_status_t jpeg_encoder_init(jpeg_encoder_t *enc, const jpeg_config_t *config);

// Encode entire image (blocking)
jpeg_status_t jpeg_encoder_encode(jpeg_encoder_t *enc);

// Encode one MCU (streaming)
jpeg_status_t jpeg_encoder_encode_mcu(jpeg_encoder_t *enc, bool *done);

// Get output size
uint32_t jpeg_encoder_get_output_size(jpeg_encoder_t *enc);

// Reset for new image
jpeg_status_t jpeg_encoder_reset(jpeg_encoder_t *enc);

// Initialize quantization reciprocals for given quality
void jpeg_init_quant_tables(uint8_t quality, jpeg_encoder_t *enc);
```

---

## Processing Pipeline

```
┌──────────────────────────────────────────────────────────────────┐
│                        MCU PROCESSING                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  1. COPY BLOCK                                                    │
│     jpeg_block_copy() → Extract 8×8 block with edge replication  │
│     [Pluggable: simple memcpy | EDMA 2D]                         │
│     Output: int16_t block[64] with level-shift (-128) applied    │
│                                                                   │
│  2. COLOR CONVERT                                                 │
│     RGB → YCbCr conversion                                        │
│     [Optional SIMD optimization]                                  │
│                                                                   │
│  3. DCT                                                           │
│     8×8 Forward DCT Type-II using fixed-point AAN algorithm      │
│     In-place: block[64] input → coefficient output               │
│     [Conditional: ARM = __SSAT/__SMLAD intrinsics,               │
│                 Windows = reference C]                            │
│                                                                   │
│  4. QUANTIZE + ZIGZAG                                             │
│     coeff = (block[i] * quant_recip[i]) >> 16                     │
│     zigzag reorder: block[zigzag[i]] → zz_block[i]               │
│                                                                   │
│  5. DC DIFFERENTIAL                                               │
│     diff = DC - prev_dc, update prev_dc                          │
│                                                                   │
│  6. HUFFMAN ENCODE                                                │
│     DC: Huffman encode difference using lookup table              │
│     AC: Run-length + Huffman encode using lookup table            │
│     Output: Bits accumulated in 32-bit buffer                     │
│                                                                   │
│  7. OUTPUT                                                        │
│     Flush 16-bit chunks from bit_buffer to output                 │
│     Handle 0xFF stuffing (0xFF → 0xFF 0x00)                      │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

---

## Cross-Platform Support

### DCT Implementation

```c
// jpeg_config.h

#if defined(__ARM_ARCH_7EM__) && defined(__ARM_FEATURE_DSP)
    #define JPEG_USE_ARM_DSP  1
    // Uses __SSAT and __SMLAD intrinsics for AAN DCT
#else
    #define JPEG_USE_ARM_DSP  0
#endif
```

```c
// jpeg_dct.c

#if JPEG_USE_ARM_DSP
    // Fast AAN DCT using 16-bit fixed-point with DSP extensions
    // 1D DCT: 8 multiplies, 26 additions (scaled)
    void jpeg_dct8x8_arm(const int16_t *block);
#else
    // Reference C implementation using 32-bit intermediates
    void jpeg_dct8x8_ref(int16_t *block);  // In-place
#endif
```

**Note on FPU**: While the AT32F437 has an FPU, fixed-point integer DCT is 2-3× faster than single-precision float for this algorithm and avoids FPU context save overhead in interrupt handlers.

### Block Copy Selection

```c
// Default selection
#ifdef JPEG_USE_EDMA
    #define JPEG_DEFAULT_BLOCK_COPY  jpeg_block_copy_edma
#else
    #define JPEG_DEFAULT_BLOCK_COPY  jpeg_block_copy_simple
#endif
```

---

## JPEG Header Structure

| Marker | Code | Description |
|--------|------|-------------|
| SOI | 0xFFD8 | Start of Image |
| APP0 | 0xFFE0 | JFIF marker (16 bytes) |
| DQT | 0xFFDB | Quantization tables |
| SOF0 | 0xFFC0 | Start of Frame (Baseline) |
| DHT | 0xFFC4 | Huffman tables |
| SOS | 0xFFDA | Start of Scan |
| EOI | 0xFFD9 | End of Image |

### Huffman Table Format

Huffman tables are pre-decoded into fast lookup structures:

```c
typedef struct {
    uint8_t  symbol;        // For DC: category (0-11). For AC: (run<<4)|category
    uint8_t  code_len;      // Actual bit length (1-16)
    uint16_t code;          // Left-aligned bit pattern
} huff_entry_t;

// DC tables: 12 entries (categories 0-11)
// AC tables: 256 entries indexed by (run<<4)|size for O(1) lookup
huff_entry_t huff_dc_y[12];
huff_entry_t huff_ac_y[256];
```

### Header Size Estimate

| Component | Size |
|-----------|------|
| SOI | 2 bytes |
| APP0 | 16 bytes |
| DQT (2 tables) | 132 bytes |
| SOF0 | 17 bytes (YUV420) |
| DHT (4 tables) | 432 bytes (with pre-computed lengths) |
| SOS | 12 bytes |
| **Total** | ~611 bytes |

---

## Testing Strategy

### Unit Tests

| Module | Test Cases |
|--------|------------|
| `jpeg_color.c` | RGB→YCbCr accuracy, boundary values |
| `jpeg_dct.c` | DCT forward/inverse consistency, fixed-point precision |
| `jpeg_quantize.c` | Zigzag order, quantization accuracy |
| `jpeg_huffman.c` | DC/AC encoding, bit stream output, 0xFF stuffing |
| `jpeg_block_copy.c` | Stride handling, edge replication padding |
| `jpeg_header.c` | Header byte-level correctness |

### End-to-End Tests

```
1. Generate test image (solid color, gradient, pattern)
2. Encode with this encoder
3. Save as .jpg file
4. Verify with external tool:
   - Python PIL/Pillow: open and decode
   - libjpeg: djpeg utility
   - ImageMagick: identify utility
5. Compare decoded pixels with original
```

### Test Verification Script

```python
# tools/verify_jpeg.py
# Usage: uv run python tools/verify_jpeg.py output.jpg original.raw

import sys
from PIL import Image
import numpy as np

def verify_jpeg(jpeg_path, original_path=None, tolerance=5):
    """Verify JPEG file is valid and optionally compare with original."""
    
    # Try to open and decode
    try:
        img = Image.open(jpeg_path)
        img.load()
    except Exception as e:
        print(f"FAIL: Cannot decode JPEG - {e}")
        return False
    
    print(f"PASS: Valid JPEG {img.size[0]}x{img.size[1]}")
    
    # Compare with original if provided
    if original_path:
        orig = Image.open(original_path)
        if img.size != orig.size:
            print(f"FAIL: Size mismatch")
            return False
        
        # Pixel comparison with tolerance
        diff = np.abs(np.array(img, dtype=int) - np.array(orig, dtype=int))
        max_diff = np.max(diff)
        mean_diff = np.mean(diff)
        
        print(f"Max difference: {max_diff}")
        print(f"Mean difference: {mean_diff:.2f}")
        
        if max_diff > tolerance:
            print(f"FAIL: Difference exceeds tolerance ({tolerance})")
            return False
    
    return True

if __name__ == "__main__":
    verify_jpeg(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
```

### CMake Test Configuration

```cmake
# CMakeLists.txt

enable_testing()

# Unit tests
add_executable(test_unit
    test/test_main.c
    test/test_unit.c
    src/jpeg_encoder.c
    src/jpeg_color.c
    src/jpeg_dct.c
    src/jpeg_quantize.c
    src/jpeg_huffman.c
    src/jpeg_tables.c
)
target_include_directories(test_unit PRIVATE include)
add_test(NAME UnitTests COMMAND test_unit)

# End-to-end tests
add_executable(test_e2e
    test/test_main.c
    test/test_e2e.c
    src/jpeg_encoder.c
    src/jpeg_header.c
    src/jpeg_color.c
    src/jpeg_dct.c
    src/jpeg_quantize.c
    src/jpeg_huffman.c
    src/jpeg_tables.c
    src/jpeg_block_copy.c
)
target_include_directories(test_e2e PRIVATE include)
add_test(NAME E2ETests COMMAND test_e2e)
```

---

## Performance Targets (AT32F437 @ 288MHz)

| Image Size | Target Time | Target Output |
|------------|-------------|---------------|
| VGA (640×480) | < 50ms | 30-50 KB |
| 1MP (1280×720) | < 150ms | 80-150 KB |
| 2MP (1920×1080) | < 300ms | 150-300 KB |
| 5MP (2592×1944) | < 800ms | 300-600 KB |

*Quality = 75, YUV420 subsampling*

---

## Memory Requirements

| Buffer | Size |
|--------|------|
| Input frame (RGB888, 5MP) | 15.1 MB |
| Output buffer (worst case) | 1 MB |
| Encoder workspace | 384 bytes |
| Quantization reciprocals | 256 bytes |
| Huffman lookup tables | ~550 bytes |
| **Total** | ~16.5 MB |

---

## Usage Example

```c
#include "jpeg_encoder.h"

int main(void)
{
    jpeg_encoder_t encoder;
    jpeg_config_t config = {
        .width = 640,
        .height = 480,
        .input_format = JPEG_FMT_RGB888,
        .subsample_mode = JPEG_SUBSAMPLE_420,
        .quality = 75,
        .input_buffer = camera_frame,
        .output_buffer = output_buffer,
        .output_buffer_size = 1024 * 1024,
        .block_copy = NULL  // Use default
    };
    
    jpeg_encoder_init(&encoder, &config);
    jpeg_encoder_encode(&encoder);
    
    uint32_t size = jpeg_encoder_get_output_size(&encoder);
    transfer_image(output_buffer, size);
    
    return 0;
}
```

---

## Dependencies

| Dependency | Platform | Purpose |
|------------|----------|---------|
| ARM DSP Intrinsics | ARM only | `__SSAT`, `__SMLAD` for fast fixed-point DCT |
| None | Windows/Other | Reference C implementation |
| Python + PIL (via uv) | Test only | JPEG verification |

---

## Build Commands

```bash
# Windows (MinGW/MSVC)
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest --output-on-failure

# ARM (AT32F437)
mkdir build_arm && cd build_arm
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
cmake --build .
```

---

## Key Implementation Notes

1. **Fixed-Point DCT**: Uses AAN (Arai/Agui/Nakajima) algorithm with 16-bit coefficients and 32-bit accumulators. In-place operation overwrites `block[64]`.

2. **Quantization**: Uses reciprocal multiplication (`(coeff * recip) >> 16`) instead of division for 12-cycle speedup per coefficient on Cortex-M4.

3. **Streaming Support**: `current_block_in_mcu` tracks position within multi-block MCUs (4:2:0 = 6 blocks) to allow resuming encoding after interruptions.

4. **Huffman Efficiency**: 32-bit `bit_buffer` accumulates up to 24 bits before flushing to memory, reducing store operations by 3× compared to byte-wise output.

5. **Edge Handling**: Block copy function replicates edge pixels for MCUs at image boundaries, ensuring all DCT inputs are full 8×8 blocks.
```
