# Agent Rules for Jpeg32 library

## Build Commands

### Windows (MSVC - run from Developer Command Prompt or Visual Studio terminal)

```
mkdir build && cd build
cmake ..
cmake --build .
```

# ARM (AT32F437)

```
mkdir build_arm && cd build_arm
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
cmake --build .
```

## Test Commands

### Run all tests

`ctest --output-on-failure`

### Run specific test

```
./test_unit
./test_e2e
```

### Verify JPEG output

`python tools/verify_jpeg.py output.jpg original.raw`

## Coding Rules

* Language: C17 compatible, no C++ features
* Style: K&R braces, 4-space indent, no tabs
* Naming: snake_case for functions/variables, UPPER_CASE for macros
* Headers: Include guards: #ifndef MODULE_H #define MODULE_H ... #endif
* Types: Use <stdint.h> types (uint8_t, uint16_t, uint32_t, int16_t)
* structs and enums: declare as typedef struct_type { ... } struct_type_t
* Memory: No dynamic allocation (malloc/free), use static/pool buffers
* Platform: Use #if JPEG_USE_CMSIS_DSP for platform-specific code
* Error handling: Return jpeg_status_t enum, check all return values
* Constants: No magic numbers, use named constants (defines) or enums
* Functions: Keep functions under 50 lines (when possible), single responsibility
* Code correctness: No warnings allowed with -Wall (or MSVC equivalent)

## Commit Rules

Review diff before commit: git diff or git diff --staged
Atomic commits: One logical change per commit
Never commit: Secrets, build artifacts, IDE files
Verify: All tests pass before commit

## File Organization

include/    - Public headers
src/        - Implementation files
test/       - Test files
tools/      - Scripts and utilities
cmake/      - CMake toolchain files

## Pre-Commit Checklist

* No compiler warnings
* Tests pass locally
* Diff reviewed
* Commit message clear
