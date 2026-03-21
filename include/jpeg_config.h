/**
 * @file jpeg_config.h
 * @brief JPEG encoder compile-time configuration
 *
 * Target: AT32F437 Cortex-M4F @ 288MHz
 */

#ifndef JPEG_CONFIG_H
#define JPEG_CONFIG_H

#include <stdint.h>

/* ============================================================================
 * Quality Settings
 * ========================================================================= */

/**
 * Default JPEG quality (1-100)
 * Value of 75 provides good balance of quality and compression
 */
#define JPEG_QUALITY_DEFAULT    75

/**
 * Minimum allowed quality value
 */
#define JPEG_QUALITY_MIN        1

/**
 * Maximum allowed quality value
 */
#define JPEG_QUALITY_MAX        100

/* ============================================================================
 * Block and MCU Dimensions
 * ========================================================================= */

/**
 * DCT block size (8x8)
 */
#define JPEG_BLOCK_SIZE         8

/**
 * MCU dimensions for 4:4:4 subsampling (Y, Cb, Cr = 1 block each)
 */
#define JPEG_MCU_WIDTH_444      8
#define JPEG_MCU_HEIGHT_444     8

/**
 * MCU dimensions for 4:2:0 subsampling (Y = 4 blocks, Cb, Cr = 1 block each)
 */
#define JPEG_MCU_WIDTH_420      16
#define JPEG_MCU_HEIGHT_420     16

/**
 * MCU dimensions for 4:2:2 subsampling (Y = 2 blocks, Cb, Cr = 1 block each)
 */
#define JPEG_MCU_WIDTH_422      16
#define JPEG_MCU_HEIGHT_422     8

/**
 * MCU dimensions for grayscale (Y only)
 */
#define JPEG_MCU_WIDTH_GRAY     8
#define JPEG_MCU_HEIGHT_GRAY    8

/* ============================================================================
 * Platform Detection
 * ========================================================================= */

/**
 * ARM DSP extension detection
 * Set to 1 when compiling for ARM architecture with DSP extensions
 */
#if defined(__ARM_ARCH_7EM__) && defined(__ARM_FEATURE_DSP)
    #define JPEG_USE_ARM_DSP     1
#else
    #define JPEG_USE_ARM_DSP     0
#endif

/* ============================================================================
 * Block Copy Selection
 * ========================================================================= */

/**
 * Default block copy function selection
 * Use EDMA version if JPEG_USE_EDMA is defined, otherwise simple memcpy
 */
#ifdef JPEG_USE_EDMA
    #define JPEG_DEFAULT_BLOCK_COPY    jpeg_block_copy_edma
#else
    #define JPEG_DEFAULT_BLOCK_COPY    jpeg_block_copy_simple
#endif

/* ============================================================================
 * Debug and Trace
 * ========================================================================= */

#ifdef JPEG_DEBUG
    #include <stdio.h>
    #define JPEG_TRACE(fmt, ...)    printf("[JPEG] " fmt "\n", ##__VA_ARGS__)
    #define JPEG_TRACE_BLOCK(block)  jpeg_trace_block(block)
#else
    #define JPEG_TRACE(fmt, ...)    ((void)0)
    #define JPEG_TRACE_BLOCK(block) ((void)0)
#endif

/**
 * Assert macro for debugging
 */
#ifdef JPEG_DEBUG
    #define JPEG_ASSERT(cond) \
        do { \
            if (!(cond)) { \
                while (1) { /* hang */ } \
            } \
        } while (0)
#else
    #define JPEG_ASSERT(cond)    ((void)0)
#endif

/* ============================================================================
 * Fixed-Point Arithmetic
 * ========================================================================= */

/**
 * Number of fractional bits for fixed-point color conversion
 * Using 14 bits provides adequate precision for YCbCr conversion
 */
#define JPEG_COLOR_FRAC_BITS    14

/**
 * Fixed-point conversion factor: 0.299 * 2^14
 */
#define JPEG_CR_FACTOR          4898

/**
 * Fixed-point conversion factor: 0.587 * 2^14
 */
#define JPEG_CG_FACTOR          9610

/**
 * Fixed-point conversion factor: 0.114 * 2^14
 */
#define JPEG_CB_FACTOR          1868

#endif /* JPEG_CONFIG_H */
