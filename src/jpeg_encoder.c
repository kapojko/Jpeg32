/**
 * @file jpeg_encoder.c
 * @brief Main JPEG encoder implementation
 *
 * Orchestrates the complete JPEG encoding pipeline: header construction,
 * MCU encoding, DCT transformation, quantization, Huffman entropy coding,
 * and bitstream output.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "jpeg_encoder.h"
#include "jpeg_tables.h"
#include "jpeg_config.h"
#include "jpeg_header.h"
#include "jpeg_color.h"
#include "jpeg_dct.h"
#include "jpeg_quantize.h"
#include "jpeg_huffman.h"
#include "jpeg_block_copy.h"

/* ============================================================================
 * MCU Block Counts by Subsampling Mode
 * ========================================================================= */

/**
 * Number of 8x8 blocks per MCU for each subsampling mode
 */
#define BLOCKS_PER_MCU_444    3   /**< Y, Cb, Cr - 3 blocks */
#define BLOCKS_PER_MCU_422    4   /**< Y1, Y2, Cb, Cr - 4 blocks */
#define BLOCKS_PER_MCU_420    6   /**< Y1-Y4, Cb, Cr - 6 blocks */
#define BLOCKS_PER_MCU_GRAY   1   /**< Y only - 1 block */

/* ============================================================================
 * Encoder Initialization
 * ========================================================================= */

jpeg_status_t jpeg_encoder_init(jpeg_encoder_t *enc, const jpeg_config_t *config)
{
    /* Validate parameters */
    if (enc == NULL || config == NULL) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    if (config->width == 0 || config->height == 0) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    if (config->quality < 1 || config->quality > 100) {
        return JPEG_STATUS_ERROR_QUALITY_OUT_OF_RANGE;
    }

    if (config->input_buffer == NULL || config->output_buffer == NULL) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    if (config->output_buffer_size == 0) {
        return JPEG_STATUS_ERROR_BUFFER_TOO_SMALL;
    }

    /* Validate input format */
    switch (config->input_format) {
        case JPEG_FMT_RGB888:
        case JPEG_FMT_RGB565:
        case JPEG_FMT_YUYV:
        case JPEG_FMT_YUV420:
        case JPEG_FMT_GRAYSCALE:
            break;
        default:
            return JPEG_STATUS_ERROR_UNSUPPORTED_FORMAT;
    }

    /* Validate subsampling mode */
    switch (config->subsample_mode) {
        case JPEG_SUBSAMPLE_444:
        case JPEG_SUBSAMPLE_420:
        case JPEG_SUBSAMPLE_422:
        case JPEG_SUBSAMPLE_GRAY:
            break;
        default:
            return JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE;
    }

    /* Grayscale must use GRAY subsampling */
    if (config->input_format == JPEG_FMT_GRAYSCALE &&
        config->subsample_mode != JPEG_SUBSAMPLE_GRAY) {
        return JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE;
    }

    /* Copy configuration to encoder state */
    enc->config = *config;

    /* Calculate MCU dimensions from subsampling mode */
    switch (config->subsample_mode) {
        case JPEG_SUBSAMPLE_444:
            enc->mcu_width = JPEG_MCU_WIDTH_444;
            enc->mcu_height = JPEG_MCU_HEIGHT_444;
            break;
        case JPEG_SUBSAMPLE_420:
            enc->mcu_width = JPEG_MCU_WIDTH_420;
            enc->mcu_height = JPEG_MCU_HEIGHT_420;
            break;
        case JPEG_SUBSAMPLE_422:
            enc->mcu_width = JPEG_MCU_WIDTH_422;
            enc->mcu_height = JPEG_MCU_HEIGHT_422;
            break;
        case JPEG_SUBSAMPLE_GRAY:
            enc->mcu_width = JPEG_MCU_WIDTH_GRAY;
            enc->mcu_height = JPEG_MCU_HEIGHT_GRAY;
            break;
        default:
            return JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE;
    }

    /* Calculate MCU counts */
    enc->mcu_count_x = (config->width + enc->mcu_width - 1) / enc->mcu_width;
    enc->mcu_count_y = (config->height + enc->mcu_height - 1) / enc->mcu_height;
    enc->total_mcus = (uint32_t)enc->mcu_count_x * (uint32_t)enc->mcu_count_y;

    /* Initialize quantization tables */
    jpeg_init_quant_tables(config->quality, enc);

    /* Initialize DC predictors */
    enc->prev_dc_y = 0;
    enc->prev_dc_cb = 0;
    enc->prev_dc_cr = 0;

    /* Initialize runtime state */
    enc->current_mcu = 0;
    enc->current_block_in_mcu = 0;
    enc->output_bytes = 0;

    /* Initialize bit buffer */
    enc->bit_buffer = 0;
    enc->bits_in_buffer = 0;

    /* Set default block copy function if not provided */
    if (enc->config.block_copy == NULL) {
        enc->config.block_copy = JPEG_DEFAULT_BLOCK_COPY;
    }

    return JPEG_STATUS_OK;
}

/* ============================================================================
 * Bitstream Output Helpers
 * ========================================================================= */

/**
 * Write bits to output buffer
 *
 * Accumulates bits and flushes bytes with 0xFF byte stuffing.
 *
 * @param enc   Encoder state
 * @param bits  Bits to write
 * @param len   Number of bits (1-16)
 */
static void write_bits(jpeg_encoder_t *enc, uint16_t bits, uint8_t len)
{
    /* Add bits to buffer (MSB first) */
    enc->bit_buffer = (enc->bit_buffer << len) | (bits & ((1U << len) - 1));
    enc->bits_in_buffer += len;

    /* Flush bytes when we have 16 or more bits */
    while (enc->bits_in_buffer >= 8) {
        uint8_t byte;

        /* Extract MSB byte */
        byte = (uint8_t)(enc->bit_buffer >> (enc->bits_in_buffer - 8));
        enc->bits_in_buffer -= 8;

        /* Write byte to output */
        enc->config.output_buffer[enc->output_bytes++] = byte;

        /* Byte stuffing: 0xFF requires 0x00 following */
        if (byte == 0xFF) {
            enc->config.output_buffer[enc->output_bytes++] = 0x00;
        }
    }
}

/**
 * Flush any remaining bits in the buffer
 *
 * Pads with 1s as required by JPEG spec.
 *
 * @param enc Encoder state
 */
static void flush_bits(jpeg_encoder_t *enc)
{
    if (enc->bits_in_buffer > 0) {
        uint8_t pad_bits;
        uint8_t byte;

        /* Pad with 1s to fill byte (JPEG spec) */
        pad_bits = 8 - enc->bits_in_buffer;
        enc->bit_buffer = (enc->bit_buffer << pad_bits) | ((1U << pad_bits) - 1);
        enc->bits_in_buffer = 8;

        /* Extract byte */
        byte = (uint8_t)(enc->bit_buffer >> (enc->bits_in_buffer - 8));
        enc->bits_in_buffer -= 8;

        /* Write byte */
        enc->config.output_buffer[enc->output_bytes++] = byte;

        /* Byte stuffing for 0xFF */
        if (byte == 0xFF) {
            enc->config.output_buffer[enc->output_bytes++] = 0x00;
        }
    }
}

/* ============================================================================
 * MCU Encoding
 * ========================================================================= */

/**
 * Encode a single 8x8 block through DCT, quantize, and Huffman
 *
 * @param enc        Encoder state
 * @param block      8x8 block data (input, gets overwritten)
 * @param is_luma    true for Y block, false for Cb/Cr
 * @param prev_dc    Previous DC coefficient (for differential encoding)
 * @return New DC value for this block
 */
static int16_t encode_block(
    jpeg_encoder_t *enc,
    int16_t *block,
    bool is_luma,
    int16_t prev_dc)
{
    int16_t quantized[64];
    int16_t dc_diff;
    int16_t dc_value;
    const huff_entry_t *dc_table;
    const huff_entry_t *ac_table;
    const uint16_t *recip;

    /* Select appropriate tables */
    if (is_luma) {
        dc_table = huff_dc_y;
        ac_table = huff_ac_y;
        recip = enc->quant_recip_y;
    } else {
        dc_table = huff_dc_c;
        ac_table = huff_ac_c;
        recip = enc->quant_recip_c;
    }

    /* Apply DCT */
    jpeg_dct8x8(block);

    /* Quantize and zigzag reorder */
    jpeg_quantize_zigzag(block, recip, quantized);

    /* Encode DC coefficient */
    dc_value = quantized[0];
    dc_diff = dc_value - prev_dc;
    jpeg_huff_encode_dc(dc_diff, dc_table, &enc->bit_buffer, &enc->bits_in_buffer);

    /* Encode AC coefficients */
    jpeg_huff_encode_ac(quantized, ac_table, &enc->bit_buffer, &enc->bits_in_buffer);

    return dc_value;
}

/**
 * Encode MCU at given position (for 4:2:0 subsampling)
 *
 * Handles 6 blocks: Y1, Y2, Y3, Y4, Cb, Cr
 *
 * @param enc   Encoder state
 * @param mcu_x MCU X position
 * @param mcu_y MCU Y position
 */
static void encode_mcu_420(jpeg_encoder_t *enc, uint16_t mcu_x, uint16_t mcu_y)
{
    uint16_t img_w = enc->config.width;
    uint16_t img_h = enc->config.height;
    uint16_t x, y;
    int16_t *block = enc->block;

    /* Y1: position (mcu_x*16, mcu_y*16) */
    x = mcu_x * 16;
    y = mcu_y * 16;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Y2: position (mcu_x*16+8, mcu_y*16) */
    x = mcu_x * 16 + 8;
    y = mcu_y * 16;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Y3: position (mcu_x*16, mcu_y*16+8) */
    x = mcu_x * 16;
    y = mcu_y * 16 + 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Y4: position (mcu_x*16+8, mcu_y*16+8) */
    x = mcu_x * 16 + 8;
    y = mcu_y * 16 + 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Cb: position (mcu_x*8, mcu_y*8) - 2x2 upsampled from 4x4 */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cb = encode_block(enc, block, false, enc->prev_dc_cb);

    /* Cr: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cr = encode_block(enc, block, false, enc->prev_dc_cr);
}

/**
 * Encode MCU at given position (for 4:2:2 subsampling)
 *
 * Handles 4 blocks: Y1, Y2, Cb, Cr
 *
 * @param enc   Encoder state
 * @param mcu_x MCU X position
 * @param mcu_y MCU Y position
 */
static void encode_mcu_422(jpeg_encoder_t *enc, uint16_t mcu_x, uint16_t mcu_y)
{
    uint16_t img_w = enc->config.width;
    uint16_t img_h = enc->config.height;
    uint16_t x, y;
    int16_t *block = enc->block;

    /* Y1: position (mcu_x*16, mcu_y*8) */
    x = mcu_x * 16;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Y2: position (mcu_x*16+8, mcu_y*8) */
    x = mcu_x * 16 + 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Cb: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cb = encode_block(enc, block, false, enc->prev_dc_cb);

    /* Cr: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cr = encode_block(enc, block, false, enc->prev_dc_cr);
}

/**
 * Encode MCU at given position (for 4:4:4 subsampling)
 *
 * Handles 3 blocks: Y, Cb, Cr
 *
 * @param enc   Encoder state
 * @param mcu_x MCU X position
 * @param mcu_y MCU Y position
 */
static void encode_mcu_444(jpeg_encoder_t *enc, uint16_t mcu_x, uint16_t mcu_y)
{
    uint16_t img_w = enc->config.width;
    uint16_t img_h = enc->config.height;
    uint16_t x, y;
    int16_t *block = enc->block;

    /* Y: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);

    /* Cb: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cb = encode_block(enc, block, false, enc->prev_dc_cb);

    /* Cr: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_cr = encode_block(enc, block, false, enc->prev_dc_cr);
}

/**
 * Encode MCU at given position (for grayscale)
 *
 * Handles 1 block: Y only
 *
 * @param enc   Encoder state
 * @param mcu_x MCU X position
 * @param mcu_y MCU Y position
 */
static void encode_mcu_gray(jpeg_encoder_t *enc, uint16_t mcu_x, uint16_t mcu_y)
{
    uint16_t img_w = enc->config.width;
    uint16_t img_h = enc->config.height;
    uint16_t x, y;
    int16_t *block = enc->block;

    /* Y: position (mcu_x*8, mcu_y*8) */
    x = mcu_x * 8;
    y = mcu_y * 8;
    enc->config.block_copy(enc->config.input_buffer, block,
                           enc->config.input_stride, x, y, img_w, img_h);
    enc->prev_dc_y = encode_block(enc, block, true, enc->prev_dc_y);
}

/* ============================================================================
 * Public API Implementation
 * ========================================================================= */

jpeg_status_t jpeg_encoder_encode(jpeg_encoder_t *enc)
{
    if (enc == NULL) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    /* Build JPEG header */
    jpeg_build_header(enc);

    /* Reset MCU counter */
    enc->current_mcu = 0;

    /* Encode all MCUs */
    while (enc->current_mcu < enc->total_mcus) {
        bool done;
        jpeg_status_t status;

        status = jpeg_encoder_encode_mcu(enc, &done);
        if (status != JPEG_STATUS_OK) {
            return status;
        }

        (void)done; /* Not needed in blocking mode */
    }

    /* Flush bitstream at end */
    flush_bits(enc);

    /* Write EOI marker */
    jpeg_write_eoi(enc);

    return JPEG_STATUS_OK;
}

jpeg_status_t jpeg_encoder_encode_mcu(jpeg_encoder_t *enc, bool *done)
{
    uint16_t mcu_x;
    uint16_t mcu_y;

    if (enc == NULL || done == NULL) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    /* Check if encoding complete */
    if (enc->current_mcu >= enc->total_mcus) {
        *done = true;
        return JPEG_STATUS_OK;
    }

    /* Calculate MCU position */
    mcu_x = enc->current_mcu % enc->mcu_count_x;
    mcu_y = enc->current_mcu / enc->mcu_count_x;

    /* Encode based on subsampling mode */
    switch (enc->config.subsample_mode) {
        case JPEG_SUBSAMPLE_420:
            encode_mcu_420(enc, mcu_x, mcu_y);
            break;
        case JPEG_SUBSAMPLE_422:
            encode_mcu_422(enc, mcu_x, mcu_y);
            break;
        case JPEG_SUBSAMPLE_444:
            encode_mcu_444(enc, mcu_x, mcu_y);
            break;
        case JPEG_SUBSAMPLE_GRAY:
            encode_mcu_gray(enc, mcu_x, mcu_y);
            break;
        default:
            return JPEG_STATUS_ERROR_UNSUPPORTED_SUBSAMPLE;
    }

    /* Advance to next MCU */
    enc->current_mcu++;
    enc->current_block_in_mcu = 0;

    /* Set done flag */
    *done = (enc->current_mcu >= enc->total_mcus);

    return JPEG_STATUS_OK;
}

uint32_t jpeg_encoder_get_output_size(jpeg_encoder_t *enc)
{
    if (enc == NULL) {
        return 0;
    }

    return enc->output_bytes;
}

jpeg_status_t jpeg_encoder_reset(jpeg_encoder_t *enc)
{
    if (enc == NULL) {
        return JPEG_STATUS_ERROR_INVALID_PARAM;
    }

    /* Reset MCU counter */
    enc->current_mcu = 0;
    enc->current_block_in_mcu = 0;

    /* Reset DC predictors */
    enc->prev_dc_y = 0;
    enc->prev_dc_cb = 0;
    enc->prev_dc_cr = 0;

    /* Reset bit buffer */
    enc->bit_buffer = 0;
    enc->bits_in_buffer = 0;

    /* Reset output size */
    enc->output_bytes = 0;

    return JPEG_STATUS_OK;
}
