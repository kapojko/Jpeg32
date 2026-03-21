/**
 * @file jpeg_quantize.h
 * @brief JPEG quantization and zigzag reordering
 *
 * Provides quantization with reciprocal multiplication for speed.
 */
#ifndef JPEG_QUANTIZE_H
#define JPEG_QUANTIZE_H

#include <stdint.h>
#include "jpeg_encoder.h"

/**
 * Initialize quantization tables for given quality
 *
 * Scales base quantization tables and computes reciprocal multipliers.
 *
 * @param quality Quality factor (1-100)
 * @param enc Encoder state (quant_recip tables updated in place)
 */
void jpeg_init_quant_tables(uint8_t quality, jpeg_encoder_t *enc);

/**
 * Quantize and reorder 8x8 block in one pass
 *
 * @param block Input DCT coefficients
 * @param recip Quantization reciprocals
 * @param quantized Output buffer for quantized zigzag-ordered coefficients
 */
void jpeg_quantize_zigzag(
    const int16_t *block,
    const uint16_t *recip,
    int16_t *quantized
);

/**
 * Reorder 8x8 block from natural to zigzag order
 *
 * @param block Input coefficients
 * @param zigzag Output zigzag-ordered coefficients
 */
void jpeg_zigzag_reorder(const int16_t *block, int16_t *zigzag);

/**
 * Dequantize block (for testing/debugging)
 *
 * @param quantized Quantized zigzag-ordered coefficients
 * @param recip Quantization reciprocals
 * @param block Output dequantized coefficients
 */
void jpeg_dequantize_block(
    const int16_t *quantized,
    const uint16_t *recip,
    int16_t *block
);

#endif /* JPEG_QUANTIZE_H */
