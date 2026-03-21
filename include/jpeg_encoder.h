/**
 * @file jpeg_encoder.h
 * @brief JPEG encoder public API
 *
 * Baseline JPEG encoder for embedded systems (target: AT32F437 Cortex-M4F)
 */

#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "jpeg_config.h"

/* ============================================================================
 * Enumerations
 * ========================================================================= */

/**
 * Input image format
 */
typedef enum {
    JPEG_FMT_RGB888,     /**< 24-bit RGB: R,G,B in sequential bytes */
    JPEG_FMT_RGB565,     /**< 16-bit RGB: 5 bits R, 6 bits G, 5 bits B */
    JPEG_FMT_YUYV,       /**< YUV 4:2:2 interleaved: Y0,U0,Y1,V0,... */
    JPEG_FMT_YUV420,     /**< YUV 4:2:0 planar: Y plane, then U, then V */
    JPEG_FMT_GRAYSCALE   /**< Grayscale: Y only, 8 bits per pixel */
} jpeg_format_t;

/**
 * Chroma subsampling mode
 */
typedef enum {
    JPEG_SUBSAMPLE_444,  /**< No subsampling: Y, Cb, Cr at full resolution */
    JPEG_SUBSAMPLE_420,  /**< 4:2:0: Cb, Cr subsampled 2x2 */
    JPEG_SUBSAMPLE_422,  /**< 4:2:2: Cb, Cr subsampled 2x1 */
    JPEG_SUBSAMPLE_GRAY  /**< Grayscale: Y only, no chroma */
} jpeg_subsample_t;

/**
 * Encoder status codes
 */
typedef enum {
    JPEG_STATUS_OK = 0,
    JPEG_STATUS_ERROR_INVALID_PARAM = -1,       /**< Invalid parameter */
    JPEG_STATUS_ERROR_BUFFER_TOO_SMALL = -2,    /**< Output buffer too small */
    JPEG_STATUS_ERROR_UNSUPPORTED_FORMAT = -3, /**< Unsupported input format */
    JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE = -4, /**< Unsupported subsampling */
    JPEG_STATUS_ERROR_QUALITY_OUT_OF_RANGE = -5 /**< Quality not in 1-100 */
} jpeg_status_t;

/* ============================================================================
 * Function Pointer Types
 * ========================================================================= */

/**
 * Block copy function pointer
 *
 * Copies an 8x8 block from source image to destination block buffer.
 * Handles edge replication for blocks that extend beyond image boundaries.
 * Applies level-shift (-128) to convert unsigned to signed values.
 *
 * @param src        Source image buffer
 * @param dst_block  Destination 8x8 block (int16_t for DCT input)
 * @param src_stride Source image stride (bytes per row)
 * @param x          Block origin X coordinate
 * @param y          Block origin Y coordinate
 * @param img_w      Image width in pixels
 * @param img_h      Image height in pixels
 */
typedef void (*jpeg_block_copy_fn)(
    const uint8_t *src,
    int16_t *dst_block,
    uint32_t src_stride,
    uint16_t x,
    uint16_t y,
    uint16_t img_w,
    uint16_t img_h
);

/* ============================================================================
 * Configuration Structure
 * ========================================================================= */

/**
 * Encoder configuration
 */
typedef struct {
    uint16_t            width;              /**< Image width in pixels */
    uint16_t            height;             /**< Image height in pixels */
    jpeg_format_t       input_format;       /**< Input image format */
    jpeg_subsample_t    subsample_mode;     /**< Chroma subsampling mode */
    uint8_t             quality;            /**< Quality factor (1-100) */
    const uint8_t      *input_buffer;       /**< Source image buffer */
    uint8_t            *output_buffer;      /**< Destination JPEG buffer */
    uint32_t            output_buffer_size; /**< Output buffer capacity */
    uint32_t            input_stride;       /**< Input buffer stride (bytes/row) */
    jpeg_block_copy_fn  block_copy;         /**< Block copy function (NULL=default) */
} jpeg_config_t;

/* ============================================================================
 * Encoder State Structure
 * ========================================================================= */

/**
 * Encoder state (opaque to caller)
 */
typedef struct {
    /* Configuration (copied from init) */
    jpeg_config_t       config;

    /* Derived parameters */
    uint16_t            mcu_width;
    uint16_t            mcu_height;
    uint16_t            mcu_count_x;
    uint16_t            mcu_count_y;
    uint32_t            total_mcus;

    /* Scaled quantization reciprocals for fast division */
    /* recip[k] = (65536 + quant[k]/2) / quant[k] */
    uint16_t            quant_recip_y[64];
    uint16_t            quant_recip_c[64];

    /* DC predictors (differential encoding across MCUs) */
    int16_t             prev_dc_y;
    int16_t             prev_dc_cb;
    int16_t             prev_dc_cr;

    /* Runtime state */
    uint32_t            current_mcu;
    uint8_t             current_block_in_mcu;  /**< 0-5 for 4:2:0 MCU tracking */
    uint32_t            output_bytes;

    /* Workspace: in-place DCT input/output (8x8 = 64 coefficients) */
    int16_t             block[64];

    /* Huffman bit buffer (32-bit for efficiency, accumulates up to 24 bits) */
    uint32_t            bit_buffer;
    uint8_t             bits_in_buffer;        /**< Valid bits in buffer (0-23) */
} jpeg_encoder_t;

/* ============================================================================
 * Public API
 * ========================================================================= */

/**
 * Initialize JPEG encoder with configuration
 *
 * @param enc    Encoder state structure (must be allocated by caller)
 * @param config Encoder configuration
 * @return JPEG_STATUS_OK on success, error code on failure
 */
jpeg_status_t jpeg_encoder_init(jpeg_encoder_t *enc, const jpeg_config_t *config);

/**
 * Encode entire image (blocking)
 *
 * @param enc Encoder state
 * @return JPEG_STATUS_OK on success, error code on failure
 */
jpeg_status_t jpeg_encoder_encode(jpeg_encoder_t *enc);

/**
 * Encode single MCU (for streaming/pipelined processing)
 *
 * @param enc  Encoder state
 * @param done Output: set to true when encoding complete
 * @return JPEG_STATUS_OK on success, error code on failure
 */
jpeg_status_t jpeg_encoder_encode_mcu(jpeg_encoder_t *enc, bool *done);

/**
 * Get size of encoded output in bytes
 *
 * @param enc Encoder state
 * @return Encoded image size in bytes
 */
uint32_t jpeg_encoder_get_output_size(jpeg_encoder_t *enc);

/**
 * Reset encoder for new image (reuses allocated buffers)
 *
 * @param enc Encoder state
 * @return JPEG_STATUS_OK on success, error code on failure
 */
jpeg_status_t jpeg_encoder_reset(jpeg_encoder_t *enc);

/**
 * Initialize quantization tables for given quality
 *
 * Computes reciprocal multipliers for fast quantization:
 * recip[k] = (65536 + quant[k]/2) / quant[k]
 *
 * @param quality Quality factor (1-100)
 * @param enc     Encoder state (quant_recip arrays filled)
 */
void jpeg_init_quant_tables(uint8_t quality, jpeg_encoder_t *enc);

#endif /* JPEG_ENCODER_H */
