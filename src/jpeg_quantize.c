/**
 * @file jpeg_quantize.c
 * @brief JPEG quantization and reordering functions
 *
 * Implements quantization table scaling, reciprocal computation for
 * fast division, and zigzag reordering for entropy coding.
 */

#include "jpeg_encoder.h"
#include "jpeg_tables.h"

/* ============================================================================
 * Quantization Table Scaling
 * ========================================================================= */

/**
 * Scale factor limits
 */
#define QUANT_SCALE_MIN   1
#define QUANT_SCALE_MAX   5000

/**
 * Calculate quantization table scale factor based on quality
 *
 * Quality mapping (per JPEG standard):
 * - quality < 50: scale = 5000 / quality
 * - quality >= 50: scale = 200 - 2 * quality
 *
 * @param quality  Quality factor (1-100)
 * @return Scale factor (1-5000)
 */
uint16_t jpeg_quantize_scale(uint8_t quality)
{
    uint16_t scale;

    if (quality < 50) {
        /* Lower quality = higher scale (more quantization) */
        scale = 5000 / quality;
    } else {
        /* Higher quality = lower scale (less quantization) */
        scale = 200 - (2 * quality);
    }

    /* Apply minimum scale of 1 */
    if (scale < QUANT_SCALE_MIN) {
        scale = QUANT_SCALE_MIN;
    }

    return scale;
}

/* ============================================================================
 * Quantization Table Initialization
 * ========================================================================= */

/**
 * Initialize quantization table with given quality factor
 *
 * Scales the base quantization table by the quality-dependent factor
 * and stores the scaled values.
 *
 * @param quality      Quality factor (1-100)
 * @param base_table   Base quantization table (jpeg_quant_table_y or jpeg_quant_table_c)
 * @param scaled_table Output: scaled quantization table (64 values)
 */
void jpeg_quantize_table_init(uint8_t quality, const uint8_t *base_table, uint8_t *scaled_table)
{
    uint16_t scale;
    uint8_t i;

    scale = jpeg_quantize_scale(quality);

    for (i = 0; i < 64; i++) {
        uint32_t val;
        val = (uint32_t)base_table[i] * scale;
        /* Round to nearest integer */
        scaled_table[i] = (uint8_t)((val + 50) / 100);
    }
}

/**
 * Initialize quantization tables and compute reciprocals for encoder
 *
 * Fills the encoder's quant_recip arrays with reciprocal multipliers
 * for fast division using multiplication and right-shift:
 *
 *   recip[k] = (65536 + quant[k]/2) / quant[k]
 *
 * This allows quantization to be performed as:
 *   coeff = (block[i] * recip[i]) >> 16
 *
 * @param quality  Quality factor (1-100)
 * @param enc      Encoder state (quant_recip_y and quant_recip_c filled)
 */
void jpeg_init_quant_tables(uint8_t quality, jpeg_encoder_t *enc)
{
    uint8_t i;
    uint8_t quant_y[64];
    uint8_t quant_c[64];

    /* Initialize scaled quantization tables */
    jpeg_quantize_table_init(quality, jpeg_quant_table_y, quant_y);
    jpeg_quantize_table_init(quality, jpeg_quant_table_c, quant_c);

    /* Compute reciprocals for luminance */
    for (i = 0; i < 64; i++) {
        uint32_t val;
        uint16_t q;

        q = (uint16_t)quant_y[i];
        if (q == 0) {
            q = 1;  /* Avoid division by zero */
        }
        val = (65536U + q / 2U) / q;
        enc->quant_recip_y[i] = (uint16_t)val;
    }

    /* Compute reciprocals for chrominance */
    for (i = 0; i < 64; i++) {
        uint32_t val;
        uint16_t q;

        q = (uint16_t)quant_c[i];
        if (q == 0) {
            q = 1;
        }
        val = (65536U + q / 2U) / q;
        enc->quant_recip_c[i] = (uint16_t)val;
    }
}

/* ============================================================================
 * Block Quantization
 * ========================================================================= */

/**
 * Quantize a single DCT block using reciprocal multiplication
 *
 * Uses pre-computed reciprocals for fast division:
 *   coeff = (block[i] * recip[i]) >> 16
 *
 * @param block  Input: 8x8 DCT coefficients (signed)
 * @param recip  Reciprocal multipliers (from jpeg_init_quant_tables)
 * @param output Output: quantized coefficients (signed)
 */
void jpeg_quantize_block(
    const int16_t *block,
    const uint16_t *recip,
    int16_t *output)
{
    uint8_t i;

    for (i = 0; i < 64; i++) {
        int32_t val;
        int16_t coeff;

        /* Signed multiplication and right-shift for division */
        val = (int32_t)block[i] * (int32_t)recip[i];

        /* Round by adding 32768 before shifting (equivalent to >> 16 with rounding) */
        val += 32768;
        coeff = (int16_t)(val >> 16);

        output[i] = coeff;
    }
}

/* ============================================================================
 * Zigzag Reordering
 * ========================================================================= */

/**
 * Reorder quantized block coefficients in zigzag order
 *
 * Standard JPEG zigzag order:
 *   0  1  8 16  9  2  3 10
 *  17 24 32 25 18 11  4  5
 *  12 19 26 33 40 48 41 34
 *  27 20 13  6  7 14 21 28
 *  35 42 49 56 57 50 43 36
 *  29 22 15 23 30 37 44 51
 *  58 59 52 45 38 31 39 46
 *  53 60 61 54 47 55 62 63
 *
 * This order groups low-frequency coefficients (typically non-zero)
 * before high-frequency coefficients (often zero), improving compression.
 *
 * @param block     Input: 8x8 coefficients in natural order
 * @param zigzag    Output: coefficients in zigzag order
 */
void jpeg_zigzag_reorder(const int16_t *block, int16_t *zigzag)
{
    uint8_t i;

    for (i = 0; i < 64; i++) {
        zigzag[i] = block[jpeg_zigzag[i]];
    }
}

/**
 * Reorder coefficients in natural (row-major) order
 *
 * Inverse of jpeg_zigzag_reorder.
 *
 * @param zigzag    Input: coefficients in zigzag order
 * @param block     Output: 8x8 coefficients in natural order
 */
void jpeg_zigzag_to_block(const int16_t *zigzag, int16_t *block)
{
    uint8_t i;

    for (i = 0; i < 64; i++) {
        block[jpeg_zigzag[i]] = zigzag[i];
    }
}

/* ============================================================================
 * Combined Quantize and Zigzag
 * ========================================================================= */

/**
 * Quantize DCT block and reorder in one pass
 *
 * Combines jpeg_quantize_block() and jpeg_zigzag_reorder() for efficiency.
 *
 * @param block     Input: 8x8 DCT coefficients (signed)
 * @param recip     Reciprocal multipliers
 * @param output    Output: quantized coefficients in zigzag order
 */
void jpeg_quantize_zigzag(
    const int16_t *block,
    const uint16_t *recip,
    int16_t *output)
{
    uint8_t i;

    for (i = 0; i < 64; i++) {
        int32_t val;
        int16_t coeff;
        uint8_t zigzag_idx;

        /* Get zigzag order index */
        zigzag_idx = jpeg_zigzag[i];

        /* Signed multiplication and right-shift for division */
        val = (int32_t)block[zigzag_idx] * (int32_t)recip[zigzag_idx];

        /* Round by adding 32768 before shifting */
        val += 32768;
        coeff = (int16_t)(val >> 16);

        output[i] = coeff;
    }
}

/**
 * Dequantize block (inverse of quantize)
 *
 * Performs inverse quantization:
 *   coeff = block[i] * quant[i]
 *
 * Note: This is approximate since quantization is lossy.
 *
 * @param block     Input: quantized coefficients
 * @param quant     Quantization table
 * @param output    Output: dequantized DCT coefficients
 */
void jpeg_dequantize_block(
    const int16_t *block,
    const uint8_t *quant,
    int16_t *output)
{
    uint8_t i;

    for (i = 0; i < 64; i++) {
        output[i] = (int16_t)block[i] * (int16_t)quant[i];
    }
}
