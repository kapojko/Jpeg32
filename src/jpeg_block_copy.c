/**
 * @file jpeg_block_copy.c
 * @brief Block copy implementation for MCU extraction
 *
 * Simple row-by-row copy with edge pixel replication and level-shift.
 * EDMA stubs provided when JPEG_USE_ARM_DSP is not defined.
 */

#include "jpeg_block_copy.h"

/* ============================================================================
 * Simple Block Copy (Reference Implementation)
 * ========================================================================= */

void jpeg_block_copy_simple(
    const uint8_t *src,
    int16_t *dst_block,
    uint32_t src_stride,
    uint16_t x,
    uint16_t y,
    uint16_t img_w,
    uint16_t img_h)
{
    uint8_t row;
    uint8_t col;
    uint16_t src_x;
    uint16_t src_y;
    uint16_t block_x;
    uint16_t block_y;

    for (row = 0; row < 8; row++) {
        /* Calculate source Y coordinate with edge replication */
        src_y = y + row;
        if (src_y >= img_h) {
            src_y = img_h - 1;
        }

        for (col = 0; col < 8; col++) {
            /* Calculate source X coordinate with edge replication */
            src_x = x + col;
            if (src_x >= img_w) {
                src_x = img_w - 1;
            }

            /* Calculate block position */
            block_x = col;
            block_y = row;

            /* Copy pixel with level-shift: unsigned [0,255] -> signed [-128,127] */
            dst_block[block_y * 8 + block_x] = (int16_t)src[src_y * src_stride + src_x] - 128;
        }
    }
}

/* ============================================================================
 * EDMA Block Copy (ARM-specific)
 * Implemented in source when JPEG_USE_ARM_DSP is defined.
 * Stub implementations are provided in jpeg_block_copy.h header.
 * ========================================================================= */
