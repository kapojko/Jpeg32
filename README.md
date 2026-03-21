# Jpeg32 - JPEG Encoder Library

Baseline JPEG encoder library for AT32F437 Cortex-M4F with cross-platform Windows testing support.

## Features

- Baseline JPEG encoding (DCT-based)
- Multiple color formats: RGB888, RGB565, YUYV, YUV420, Grayscale
- Configurable chroma subsampling (4:4:4, 4:2:0, 4:2:2)
- Quality control (1-100)
- ARM CMSIS-DSP optimized DCT
- No dynamic memory allocation

## Project Structure

```
Jpeg32/
├── include/          # Public headers
├── src/              # Implementation files
├── test/             # Test files
│   └── test_images/  # Test image data
├── tools/            # Scripts and utilities
├── cmake/            # CMake toolchain files
└── plans/            # Project planning documents
```

## Build Instructions

### Windows (MSVC)

Requires Visual Studio with MSVC compiler or Developer Command Prompt.

```cmd
mkdir build
cd build
cmake ..
cmake --build .
```

### ARM Cross-Compilation (AT32F437)

Requires arm-none-eabi toolchain installed.

```cmd
mkdir build_arm
cd build_arm
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
cmake --build .
```

### Build Options

- `JPEG_BUILD_TESTS`: Build test executables (default: ON)
- `JPEG_BUILD_TOOLS`: Build utility tools (default: ON)
- `CMAKE_BUILD_TYPE`: Debug or Release (default: Release)

## Testing

### Run All Tests

```cmd
ctest --output-on-failure
```

### Run Specific Tests

```cmd
./build/test/test_unit
./build/test/test_e2e
```

### Verify JPEG Output

```cmd
uv run python tools/verify_jpeg.py output.jpg original.raw
```

## Coding Standards

- Language: C17 compatible, no C++ features
- Style: K&R braces, 4-space indent, no tabs
- Naming: snake_case for functions/variables, UPPER_CASE for macros
- Headers: Include guards with `#ifndef MODULE_H`
- Types: Use `<stdint.h>` types (uint8_t, uint16_t, uint32_t, int16_t)
- Memory: No dynamic allocation (malloc/free), use static/pool buffers
- Functions: Keep under 50 lines when possible, single responsibility
- Code correctness: No warnings allowed with -Wall (or MSVC equivalent)

## Error Handling

All public functions return `jpeg_status_t` enum:
- `JPEG_STATUS_OK` - Success
- `JPEG_STATUS_ERROR` - Generic error
- `JPEG_STATUS_BAD_PARAM` - Invalid parameter
- `JPEG_STATUS_BUF_TOO_SMALL` - Output buffer insufficient
- `JPEG_STATUS_INTERNAL_ERROR` - Internal logic error

## Configuration

Edit `include/jpeg_config.h` to customize:
- `JPEG_QUALITY_DEFAULT` - Default encoding quality (1-100)
- `JPEG_USE_CMSIS_DSP` - Enable ARM DSP optimizations
- Debug/trace macros

## License

[License info]
