/**
 * @file jpeg_tables.h
 * @brief JPEG quantization and Huffman tables
 *
 * Standard ITU-R BT.601 quantization tables and pre-computed Huffman
 * lookup structures for O(1) encoding.
 */

#ifndef JPEG_TABLES_H
#define JPEG_TABLES_H

#include <stdint.h>

/* ============================================================================
 * Huffman Lookup Entry
 * ========================================================================= */

/**
 * Pre-decoded Huffman table entry for O(1) lookup
 *
 * Used to encode DC and AC coefficients:
 * - DC: symbol = category (0-11), code = bit pattern, code_len = bit length
 * - AC: symbol = (run << 4) | category, code = bit pattern, code_len = bit length
 */
typedef struct {
    uint8_t  symbol;     /**< Category (DC) or (run<<4)|category (AC) */
    uint8_t  code_len;   /**< Huffman code length in bits (1-16) */
    uint16_t code;       /**< Left-aligned Huffman code pattern */
} huff_entry_t;

/* ============================================================================
 * Quantization Tables (Base Values)
 * ========================================================================= */

/**
 * Standard luminance quantization table (quality = 50)
 * Scale up for lower quality, down for higher quality
 */
extern const uint8_t jpeg_quant_table_y[64];

/**
 * Standard chrominance quantization table (quality = 50)
 */
extern const uint8_t jpeg_quant_table_c[64];

/* ============================================================================
 * Zigzag Scan Order
 * ========================================================================= */

/**
 * Zigzag scan order for 8x8 block
 * Index i gives the original coefficient position for zigzag position i
 */
extern const uint8_t jpeg_zigzag[64];

/* ============================================================================
 * Huffman Table Raw Data
 * ========================================================================= */

/**
 * Huffman DC luminance bits (count of codes per length)
 */
extern const uint8_t huff_dc_y_bits[16];

/**
 * Huffman DC luminance values (variable-length codes)
 */
extern const int8_t huff_dc_y_vals[12];

/**
 * Huffman DC chrominance bits
 */
extern const uint8_t huff_dc_c_bits[16];

/**
 * Huffman DC chrominance values
 */
extern const int8_t huff_dc_c_vals[12];

/**
 * Huffman AC luminance bits
 */
extern const uint8_t huff_ac_y_bits[16];

/**
 * Huffman AC luminance values
 */
extern const uint8_t huff_ac_y_vals[162];

/**
 * Huffman AC chrominance bits
 */
extern const uint8_t huff_ac_c_bits[16];

/**
 * Huffman AC chrominance values
 */
extern const uint8_t huff_ac_c_vals[162];

/* ============================================================================
 * Pre-decoded Huffman Lookup Tables
 * ========================================================================= */

/**
 * DC luminance Huffman lookup (12 entries for categories 0-11)
 */
extern const huff_entry_t huff_dc_y[12];

/**
 * DC chrominance Huffman lookup
 */
extern const huff_entry_t huff_dc_c[12];

/**
 * AC luminance Huffman lookup (256 entries)
 * Indexed by (run << 4) | size for O(1) lookup
 */
extern const huff_entry_t huff_ac_y[256];

/**
 * AC chrominance Huffman lookup
 */
extern const huff_entry_t huff_ac_c[256];

/* ============================================================================
 * Color Conversion Constants (Fixed-Point)
 * ========================================================================= */

/**
 * Number of fractional bits for fixed-point color conversion
 */
#ifndef JPEG_COLOR_FRAC_BITS
#define JPEG_COLOR_FRAC_BITS  14
#endif

/**
 * Fixed-point conversion: (0.299 * 2^FRAC_BITS) for R→Y contribution
 */
#define JPEG_FX_CR             4898

/**
 * Fixed-point conversion: (0.587 * 2^FRAC_BITS) for G→Y contribution
 */
#define JPEG_FX_CG             9610

/**
 * Fixed-point conversion: (0.114 * 2^FRAC_BITS) for B→Y contribution
 */
#define JPEG_FX_CB             1868

/**
 * Fixed-point constant: 128 * 2^FRAC_BITS for level shift
 */
#define JPEG_FX_LEVEL_SHIFT    2097152

/**
 * Fixed-point constant: 128 * 2^FRAC_BITS for Cb/Pb extraction
 */
#define JPEG_FX_CB_OFFSET      2097152

/**
 * Fixed-point constant: 128 * 2^FRAC_BITS for Cr/Pr extraction
 */
#define JPEG_FX_CR_OFFSET      2097152

#endif /* JPEG_TABLES_H */
