/**
 * @file jpeg_block_copy.h
 * @brief Block copy interface for MCU extraction
 *
 * Pluggable block copy implementations:
 * - Simple memcpy for reference/testing
 * - EDMA for high-performance ARM embedded systems
 */

#ifndef JPEG_BLOCK_COPY_H
#define JPEG_BLOCK_COPY_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Simple Block Copy (Reference Implementation)
 * ========================================================================= */

/**
 * Copy 8x8 block from source image with edge replication and level-shift
 *
 * Uses simple row-by-row memcpy for each line of the block.
 * For edge blocks (partial blocks at image boundaries), replicates
 * the edge pixels instead of using zero-fill.
 * Applies level-shift (-128) to convert unsigned [0,255] to signed [-128,127].
 *
 * @param src        Source image buffer (RGB or Y plane)
 * @param dst_block  Destination 8x8 block (int16_t for DCT input)
 * @param src_stride Source image stride (bytes per row)
 * @param x          Block origin X coordinate
 * @param y          Block origin Y coordinate
 * @param img_w      Image width in pixels
 * @param img_h      Image height in pixels
 */
void jpeg_block_copy_simple(
    const uint8_t *src,
    int16_t *dst_block,
    uint32_t src_stride,
    uint16_t x,
    uint16_t y,
    uint16_t img_w,
    uint16_t img_h
);

/* ============================================================================
 * EDMA Block Copy (ARM-specific High-Performance)
 * ========================================================================= */

#if JPEG_USE_ARM_DSP

/**
 * Initialize EDMA for block copy operations
 *
 * Must be called before using jpeg_block_copy_edma.
 *
 * @return 0 on success, negative on error
 */
int jpeg_block_copy_edma_init(void);

/**
 * Copy 8x8 block using EDMA 2D transfer
 *
 * Asynchronous transfer - check completion with jpeg_block_copy_edma_is_busy().
 * Produces same output as jpeg_block_copy_simple() but faster on ARM.
 *
 * @param src        Source image buffer
 * @param dst_block  Destination 8x8 block
 * @param src_stride Source image stride
 * @param x          Block origin X
 * @param y          Block origin Y
 * @param img_w      Image width
 * @param img_h      Image height
 */
void jpeg_block_copy_edma(
    const uint8_t *src,
    int16_t *dst_block,
    uint32_t src_stride,
    uint16_t x,
    uint16_t y,
    uint16_t img_w,
    uint16_t img_h
);

/**
 * Check if EDMA transfer is in progress
 *
 * @return true if busy, false if idle
 */
bool jpeg_block_copy_edma_is_busy(void);

#else /* !JPEG_USE_ARM_DSP */

/* Stub implementations when EDMA is not available */

static inline int jpeg_block_copy_edma_init(void)
{
    return -1;  /* Not available */
}

static inline void jpeg_block_copy_edma(
    const uint8_t *src,
    int16_t *dst_block,
    uint32_t src_stride,
    uint16_t x,
    uint16_t y,
    uint16_t img_w,
    uint16_t img_h)
{
    (void)src;
    (void)dst_block;
    (void)src_stride;
    (void)x;
    (void)y;
    (void)img_w;
    (void)img_h;
}

static inline bool jpeg_block_copy_edma_is_busy(void)
{
    return false;
}

#endif /* JPEG_USE_ARM_DSP */

#endif /* JPEG_BLOCK_COPY_H */
