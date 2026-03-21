/**
 * @file jpeg_color.h
 * @brief Color conversion functions for JPEG encoding
 *
 * Provides RGB to YCbCr conversion and chroma subsampling.
 */
#ifndef JPEG_COLOR_H
#define JPEG_COLOR_H

#include <stdint.h>

/**
 * Convert single RGB pixel to YCbCr
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param y Output Y component
 * @param cb Output Cb component
 * @param cr Output Cr component
 */
void jpeg_rgb_to_ycbcr(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    int16_t *y,
    int16_t *cb,
    int16_t *cr
);

/**
 * Convert 8x8 RGB888 block to YCbCr
 *
 * @param src Source RGB888 buffer (8x8 pixels, 3 bytes per pixel)
 * @param dst_y Destination Y plane (8x8)
 * @param dst_cb Destination Cb plane (8x8)
 * @param dst_cr Destination Cr plane (8x8)
 * @param src_stride Source stride in bytes
 */
void jpeg_rgb888_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr,
    uint32_t src_stride
);

/**
 * Convert 8x8 RGB565 block to YCbCr
 *
 * @param src Source RGB565 buffer
 * @param dst_y Destination Y plane (8x8)
 * @param dst_cb Destination Cb plane (8x8)
 * @param dst_cr Destination Cr plane (8x8)
 * @param src_stride Source stride in bytes
 */
void jpeg_rgb565_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr,
    uint32_t src_stride
);

/**
 * Convert 8x8 YUYV block to YCbCr
 *
 * @param src Source YUYV buffer (4:2:2 interleaved)
 * @param dst_y Destination Y plane (8x8)
 * @param dst_cb Destination Cb plane (8x8)
 * @param dst_cr Destination Cr plane (8x8)
 * @param src_stride Source stride in bytes
 */
void jpeg_yuyv_to_ycbcr_block(
    const uint8_t *src,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr,
    uint32_t src_stride
);

/**
 * Convert 8x8 YUV420 block to YCbCr
 *
 * @param src_y Source Y plane (8x8)
 * @param src_cb Source Cb plane (4x4, 2x2 subsampled)
 * @param src_cr Source Cr plane (4x4, 2x2 subsampled)
 * @param dst_y Destination Y plane (8x8)
 * @param dst_cb Destination Cb plane (8x8)
 * @param dst_cr Destination Cr plane (8x8)
 */
void jpeg_yuv420_to_ycbcr_block(
    const uint8_t *src_y,
    const uint8_t *src_cb,
    const uint8_t *src_cr,
    int16_t *dst_y,
    int16_t *dst_cb,
    int16_t *dst_cr
);

/**
 * Subsample YCbCr from 8x8 to 4x4 (4:2:0)
 *
 * @param src_y Source Y plane (8x8)
 * @param src_cb Source Cb plane (8x8)
 * @param src_cr Source Cr plane (8x8)
 * @param dst_cb Output Cb plane (4x4, averaged)
 * @param dst_cr Output Cr plane (4x4, averaged)
 */
void jpeg_ycbcr_subsample_420(
    const int16_t *src_y,
    const int16_t *src_cb,
    const int16_t *src_cr,
    int16_t *dst_cb,
    int16_t *dst_cr
);

/**
 * Subsample YCbCr from 8x8 to 8x4 (4:2:2)
 *
 * @param src_y Source Y plane (8x8)
 * @param src_cb Source Cb plane (8x8)
 * @param src_cr Source Cr plane (8x8)
 * @param dst_cb Output Cb plane (8x4, averaged horizontally)
 * @param dst_cr Output Cr plane (8x4, averaged horizontally)
 */
void jpeg_ycbcr_subsample_422(
    const int16_t *src_y,
    const int16_t *src_cb,
    const int16_t *src_cr,
    int16_t *dst_cb,
    int16_t *dst_cr
);

#endif /* JPEG_COLOR_H */
