/**
 * @file jpeg_color.c
 * @brief Color conversion functions for JPEG encoding
 *
 * Implements RGB to YCbCr conversion and various input format conversions
 * using fixed-point arithmetic for embedded systems.
 */

#include "jpeg_encoder.h"
#include "jpeg_tables.h"

/* ============================================================================
 * Color Conversion Constants
 * ========================================================================= */

/**
 * Fixed-point offset for chroma components (128 << FRAC_BITS)
 * Used to center Cb/Cr values around zero in fixed-point arithmetic
 */
#define JPEG_CR_OFFSET_FIXED  (128 << JPEG_COLOR_FRAC_BITS)

/**
 * Fixed-point scale for Cb (0.5 * 2^FRAC_BITS)
 * Cb = 128 + (-0.168*R - 0.331*G + 0.5*B) / 1.0
 */
#define JPEG_CB_SCALE_FIXED    ((int32_t)(0.5 * (1 << JPEG_COLOR_FRAC_BITS)))

/**
 * Fixed-point scale for Cr (0.5 * 2^FRAC_BITS)
 * Cr = 128 + (0.5*R - 0.419*G - 0.081*B) / 1.0
 */
#define JPEG_CR_SCALE_FIXED   ((int32_t)(0.5 * (1 << JPEG_COLOR_FRAC_BITS)))

/* ============================================================================
 * Single Pixel Color Conversion
 * ========================================================================= */

/**
 * Convert single RGB pixel to YCbCr using fixed-point arithmetic
 *
 * @param r     Red component (0-255)
 * @param g     Green component (0-255)
 * @param y     Output: Y luminance component
 * @param cb    Output: Cb chroma component (centered, not offset)
 * @param cr    Output: Cr chroma component (centered, not offset)
 */
void jpeg_rgb_to_ycbcr(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    int16_t *y,
    int16_t *cb,
    int16_t *cr)
{
    int32_t y_val;
    int32_t cb_val;
    int32_t cr_val;
    int32_t r_centered;
    int32_t g_centered;
    int32_t b_centered;

    /* Center the RGB values around 128 */
    r_centered = (int32_t)r - 128;
    g_centered = (int32_t)g - 128;
    b_centered = (int32_t)b - 128;

    /* Y = 0.299*R + 0.587*G + 0.114*B
     * Using pre-computed fixed-point factors: Y = (4898*R + 9610*G + 1868*B) >> 14 */
    y_val = (4898 * r_centered + 9610 * g_centered + 1868 * b_centered) >> JPEG_COLOR_FRAC_BITS;

    /* Cb = 128 - 0.168*R - 0.331*G + 0.5*B
     * Simplified: Cb = (-86*R - 169*G + 256*B) >> 9 (approx) */
    cb_val = (256 * b_centered - 86 * r_centered - 169 * g_centered) >> 9;

    /* Cr = 128 + 0.5*R - 0.419*G - 0.081*B
     * Simplified: Cr = (256*R - 214*G - 42*B) >> 9 (approx) */
    cr_val = (256 * r_centered - 214 * g_centered - 42 * b_centered) >> 9;

    /* Clamp to valid range and store */
    *y  = (int16_t)(y_val < -128 ? -128 : (y_val > 127 ? 127 : y_val));
    *cb = (int16_t)(cb_val < -128 ? -128 : (cb_val > 127 ? 127 : cb_val));
    *cr = (int16_t)(cr_val < -128 ? -128 : (cr_val > 127 ? 127 : cr_val));
}

/* ============================================================================
 * 8x8 Block Color Conversion
 * ========================================================================= */

/**
 * Convert 8x8 RGB888 block to YCbCr
 *
 * Input is packed RGB: R0,G0,B0,R1,G1,B1,...
 *
 * @param src       Source RGB888 buffer (64 pixels * 3 bytes)
 * @param dst_y     Output: 8x8 Y luminance block
 * @param dst_cb    Output: 8x8 Cb chroma block
 * @param dst_cr    Output: 8x8 Cr chroma block
 */
void jpeg_rgb888_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t row;
    uint8_t col;
    uint8_t pixel_idx;
    const uint8_t *pixel;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            pixel_idx = row * 8 + col;
            pixel = src + pixel_idx * 3;

            jpeg_rgb_to_ycbcr(
                pixel[0],   /* R */
                pixel[1],   /* G */
                pixel[2],   /* B */
                &dst_y[pixel_idx],
                &dst_cb[pixel_idx],
                &dst_cr[pixel_idx]
            );
        }
    }
}

/**
 * Convert 8x8 RGB565 block to YCbCr
 *
 * Input format: 5 bits R, 6 bits G, 5 bits B (16 bits per pixel)
 * Layout: [R4 R3 R2 R1 R0 G5 G4 G3 G2 G1 G0 B4 B3 B2 B1 B0]
 *
 * @param src       Source RGB565 buffer (64 pixels * 2 bytes)
 * @param dst_y     Output: 8x8 Y luminance block
 * @param dst_cb    Output: 8x8 Cb chroma block
 * @param dst_cr    Output: 8x8 Cr chroma block
 */
void jpeg_rgb565_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t row;
    uint8_t col;
    uint8_t pixel_idx;
    uint16_t pixel;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            pixel_idx = row * 8 + col;

            /* Read 16-bit RGB565 pixel (little-endian) */
            pixel = (uint16_t)src[pixel_idx * 2] | ((uint16_t)src[pixel_idx * 2 + 1] << 8);

            /* Extract components */
            r = (uint8_t)((pixel >> 11) & 0x1F);
            g = (uint8_t)((pixel >> 5) & 0x3F);
            b = (uint8_t)(pixel & 0x1F);

            /* Scale to 8-bit: R and B are 5-bit (0-31 -> 0-255), G is 6-bit (0-63 -> 0-255) */
            r = (uint8_t)((r << 3) | (r >> 2));
            g = (uint8_t)((g << 2) | (g >> 4));
            b = (uint8_t)((b << 3) | (b >> 2));

            jpeg_rgb_to_ycbcr(r, g, b,
                &dst_y[pixel_idx],
                &dst_cb[pixel_idx],
                &dst_cr[pixel_idx]
            );
        }
    }
}

/**
 * Convert 8x8 YUYV block to YCbCr
 *
 * Input format (4:2:2 interleaved): Y0,U0,Y1,V0,Y2,U2,Y3,V3,...
 * Each pair shares chroma: (Y0,Y1) share U0,V0, (Y2,Y3) share U2,V2, etc.
 *
 * @param src       Source YUYV buffer (128 bytes: 16 Y + 8 U + 8 V pairs)
 * @param dst_y     Output: 8x8 Y luminance block
 * @param dst_cb    Output: 8x8 Cb chroma block
 * @param dst_cr    Output: 8x8 Cr chroma block
 */
void jpeg_yuyv_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t row;
    uint8_t col;
    uint8_t pixel_idx;
    uint8_t pair_idx;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            pixel_idx = row * 8 + col;
            pair_idx = col >> 1;  /* Each pair of pixels shares chroma */

            /* Y is at positions 0, 4, 8, 12... for even pixels, 2, 6, 10, 14... for odd */
            /* U is at positions 1, 5, 9, 13... (every 4 bytes, starting at 1) */
            /* V is at positions 3, 7, 11, 15... (every 4 bytes, starting at 3) */

            /* Y value - direct lookup */
            dst_y[pixel_idx] = (int16_t)src[row * 16 + col * 2] - 128;

            /* Cb and Cr - each pair of pixels shares the same chroma */
            dst_cb[pixel_idx] = (int16_t)src[row * 16 + pair_idx * 4 + 1] - 128;
            dst_cr[pixel_idx] = (int16_t)src[row * 16 + pair_idx * 4 + 3] - 128;
        }
    }
}

/**
 * Convert 8x8 YUV420 block to YCbCr
 *
 * Input format (4:2:0 planar): Y plane (64 bytes), then U plane (16 bytes),
 * then V plane (16 bytes)
 *
 * @param src_y     Source Y plane (64 bytes)
 * @param src_cb    Source Cb plane (16 bytes, 8x8 subsampled)
 * @param src_cr    Source Cr plane (16 bytes, 8x8 subsampled)
 * @param dst_y     Output: 8x8 Y luminance block
 * @param dst_cb    Output: 8x8 Cb chroma block (upsampled from 4x4)
 * @param dst_cr    Output: 8x8 Cr chroma block (upsampled from 4x4)
 */
void jpeg_yuv420_to_ycbcr_block(
    const uint8_t *src_y,
    const uint8_t *src_cb,
    const uint8_t *src_cr,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t row;
    uint8_t col;
    uint8_t pixel_idx;
    uint8_t chroma_row;
    uint8_t chroma_col;
    uint8_t chroma_idx;

    /* Copy Y with level shift */
    for (pixel_idx = 0; pixel_idx < 64; pixel_idx++) {
        dst_y[pixel_idx] = (int16_t)src_y[pixel_idx] - 128;
    }

    /* Upsample Cb from 4x4 to 8x8
     * Each chroma sample is shared by a 2x2 block of luma samples */
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            pixel_idx = row * 8 + col;
            chroma_row = row >> 1;  /* 0-3 for rows 0-1, 2-3 for rows 2-3, etc. */
            chroma_col = col >> 1;
            chroma_idx = chroma_row * 4 + chroma_col;

            dst_cb[pixel_idx] = (int16_t)src_cb[chroma_idx] - 128;
            dst_cr[pixel_idx] = (int16_t)src_cr[chroma_idx] - 128;
        }
    }
}

/* ============================================================================
 * Chroma Subsampling
 * ========================================================================= */

/**
 * Subsample YCbCr 4:4:4 to 4:2:0
 *
 * Takes four 8x8 blocks (Y, Cb, Cr at full resolution) and produces
 * one 16x16 MCU with 4 Y blocks and 1 each Cb/Cr block (subsampled 2x2).
 *
 * @param src_y     Source Y blocks (4 x 8x8 = 256 values)
 * @param src_cb    Source Cb block (64 values)
 * @param src_cr    Source Cr block (64 values)
 * @param dst_y     Output: 4 Y blocks (stored contiguously)
 * @param dst_cb    Output: subsampled Cb block (16 values, 4x4)
 * @param dst_cr    Output: subsampled Cr block (16 values, 4x4)
 */
void jpeg_ycbcr_subsample_420(
    const int16_t *src_y,
    const int16_t *src_cb,
    const int16_t *src_cr,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t block;
    uint8_t row;
    uint8_t col;
    uint8_t src_idx;
    uint8_t dst_idx;
    uint8_t chroma_row;
    uint8_t chroma_col;
    int32_t cb_sum;
    int32_t cr_sum;

    /* Copy Y blocks (4 blocks of 8x8) */
    for (block = 0; block < 4; block++) {
        for (row = 0; row < 8; row++) {
            for (col = 0; col < 8; col++) {
                src_idx = block * 64 + row * 8 + col;
                dst_idx = block * 64 + row * 8 + col;
                dst_y[dst_idx] = src_y[src_idx];
            }
        }
    }

    /* Subsample Cb and Cr from 8x8 to 4x4
     * Average each 2x2 block into one chroma sample */
    for (chroma_row = 0; chroma_row < 4; chroma_row++) {
        for (chroma_col = 0; chroma_col < 4; chroma_col++) {
            cb_sum = 0;
            cr_sum = 0;

            /* Average 2x2 block */
            for (row = 0; row < 2; row++) {
                for (col = 0; col < 2; col++) {
                    src_idx = (chroma_row * 2 + row) * 8 + (chroma_col * 2 + col);
                    cb_sum += src_cb[src_idx];
                    cr_sum += src_cr[src_idx];
                }
            }

            dst_idx = chroma_row * 4 + chroma_col;
            dst_cb[dst_idx] = (int16_t)(cb_sum >> 2);  /* Divide by 4 */
            dst_cr[dst_idx] = (int16_t)(cr_sum >> 2);
        }
    }
}

/**
 * Subsample YCbCr 4:4:4 to 4:2:2
 *
 * Takes two 8x8 Y blocks and produces one 16x8 MCU with 2 Y blocks
 * and 1 each Cb/Cr block (subsampled 2x1 horizontally).
 *
 * @param src_y     Source Y blocks (2 x 8x8 = 128 values)
 * @param src_cb    Source Cb block (64 values)
 * @param src_cr    Source Cr block (64 values)
 * @param dst_y     Output: 2 Y blocks (stored contiguously)
 * @param dst_cb    Output: subsampled Cb block (8 values, 1x8)
 * @param dst_cr    Output: subsampled Cr block (8 values, 1x8)
 */
void jpeg_ycbcr_subsample_422(
    const int16_t *src_y,
    const int16_t *src_cb,
    const int16_t *src_cr,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr)
{
    uint8_t block;
    uint8_t row;
    uint8_t col;
    uint8_t src_idx;
    uint8_t dst_idx;
    uint8_t chroma_col;
    int32_t cb_sum;
    int32_t cr_sum;

    /* Copy Y blocks (2 blocks of 8x8) */
    for (block = 0; block < 2; block++) {
        for (row = 0; row < 8; row++) {
            for (col = 0; col < 8; col++) {
                src_idx = block * 64 + row * 8 + col;
                dst_idx = block * 64 + row * 8 + col;
                dst_y[dst_idx] = src_y[src_idx];
            }
        }
    }

    /* Subsample Cb and Cr from 8x8 to 1x8 (average pairs horizontally)
     * Each chroma sample is shared by 2 horizontally adjacent luma samples */
    for (row = 0; row < 8; row++) {
        for (chroma_col = 0; chroma_col < 4; chroma_col++) {
            cb_sum = 0;
            cr_sum = 0;

            /* Average 2 horizontally adjacent samples */
            for (col = 0; col < 2; col++) {
                src_idx = row * 8 + chroma_col * 2 + col;
                cb_sum += src_cb[src_idx];
                cr_sum += src_cr[src_idx];
            }

            dst_idx = row * 4 + chroma_col;
            dst_cb[dst_idx] = (int16_t)(cb_sum >> 1);  /* Divide by 2 */
            dst_cr[dst_idx] = (int16_t)(cr_sum >> 1);
        }
    }
}
