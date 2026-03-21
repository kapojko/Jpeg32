# ARM None-EABI Toolchain File for AT32F437 (Cortex-M4F)
# Usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake

# Target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain location (adjust for your environment)
set(TOOLCHAIN_PREFIX arm-none-eabi)

# Compiler flags
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}-size)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}-objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}-objdump)

# Search programs in the toolchain
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Target architecture - Cortex-M4F with DSP instructions
set(TARGET_FLAGS "-march=armv7e-m+fp -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Standard flags
set(CMAKE_C_FLAGS_INIT "-std=c17 -ffunction-sections -fdata-sections ${TARGET_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT}" CACHE STRING "")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-T ${CMAKE_SOURCE_DIR}/cmake/linker_script.ld -march=armv7e-m+fp -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map" CACHE STRING "")

# Disable warnings as errors for system headers
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign")

# Build type specific settings
set(CMAKE_BUILD_TYPE Release CACHE STRING "")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# CMSIS-DSP support detection
set(JPEG_USE_CMSIS_DSP 1 CACHE BOOL "Use ARM CMSIS-DSP library")

if(JPEG_USE_CMSIS_DSP)
    add_definitions(-DJPEG_USE_CMSIS_DSP=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DARM_MATH_CM4")
endif()
