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

---

## File Structure

```
jpeg_encoder/
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
    const uint8_t *src, uint8_t *dst,
    uint16_t src_stride, uint16_t block_w, uint16_t block_h
);

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
    
    // Scaled quantization tables
    uint16_t            quant_y[64];
    uint16_t            quant_c[64];
    
    // DC predictors (persist across MCUs)
    int16_t             prev_dc_y;
    int16_t             prev_dc_cb;
    int16_t             prev_dc_cr;
    
    // Runtime state
    uint32_t            current_mcu;
    uint32_t            output_bytes;
    
    // Workspaces
    int16_t             block[64];
    float               dct_workspace[64];
    
    // Huffman state
    uint8_t             bit_buffer;
    uint8_t             bits_in_buffer;
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
```

---

## Processing Pipeline

```
┌──────────────────────────────────────────────────────────────────┐
│                        MCU PROCESSING                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  1. COPY BLOCK                                                    │
│     jpeg_block_copy() → Extract MCU from input buffer            │
│     [Pluggable: simple memcpy | EDMA 2D]                         │
│                                                                   │
│  2. COLOR CONVERT                                                 │
│     RGB → YCbCr conversion                                        │
│     [Optional SIMD optimization]                                  │
│                                                                   │
│  3. LEVEL SHIFT                                                   │
│     Subtract 128 from each component                              │
│                                                                   │
│  4. DCT                                                           │
│     8×8 Forward DCT using CMSIS-DSP or reference                  │
│     [Conditional: ARM = CMSIS-DSP, Windows = reference C]        │
│                                                                   │
│  5. QUANTIZE                                                      │
│     Divide by quantization table, round, zigzag reorder          │
│                                                                   │
│  6. DC DIFFERENTIAL                                               │
│     diff = DC - prev_dc, update prev_dc                          │
│                                                                   │
│  7. HUFFMAN ENCODE                                                │
│     DC: Huffman encode difference                                 │
│     AC: Run-length + Huffman encode                               │
│                                                                   │
│  8. OUTPUT                                                        │
│     Write to output buffer                                        │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

---

## Cross-Platform Support

### DCT Implementation Selection

```c
// jpeg_config.h

#if defined(__ARM_ARCH_7EM__) && defined(__FPU_PRESENT)
    #define JPEG_USE_CMSIS_DSP  1
#else
    #define JPEG_USE_CMSIS_DSP  0
#endif
```

```c
// jpeg_dct.c

#if JPEG_USE_CMSIS_DSP
    #include "arm_math.h"
    // Use arm_dct4_f32()
#else
    // Reference C implementation
    void jpeg_dct8x8_ref(const int16_t *input, int16_t *output);
#endif
```

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

### Header Size Estimate

| Component | Size |
|-----------|------|
| SOI | 2 bytes |
| APP0 | 18 bytes |
| DQT (2 tables) | 138 bytes |
| SOF0 | 17 bytes (YUV420) |
| DHT (4 tables) | 418 bytes |
| SOS | 12 bytes |
| **Total** | ~605 bytes |

---

## Testing Strategy

### Unit Tests

| Module | Test Cases |
|--------|------------|
| `jpeg_color.c` | RGB→YCbCr accuracy, boundary values |
| `jpeg_dct.c` | DCT forward/inverse consistency |
| `jpeg_quantize.c` | Zigzag order, quantization accuracy |
| `jpeg_huffman.c` | DC/AC encoding, bit stream output |
| `jpeg_block_copy.c` | Stride handling, boundary conditions |
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
| Encoder workspace | 512 bytes |
| Quantization tables | 256 bytes |
| Huffman tables | 512 bytes |
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
| CMSIS-DSP | ARM only | SIMD-optimized DCT |
| None | Windows/Other | Reference C implementation |
| Python + PIL | Test only | JPEG verification |

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
