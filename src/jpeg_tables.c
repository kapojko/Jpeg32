/**
 * @file jpeg_tables.c
 * @brief JPEG quantization and Huffman tables implementation
 *
 * Standard ITU-R BT.601 quantization tables, Huffman table definitions,
 * and pre-decoded lookup structures for O(1) encoding.
 */

#include "jpeg_tables.h"

/* ============================================================================
 * Quantization Tables (Base Values - Quality 50)
 * ========================================================================= */

/**
 * Standard luminance quantization table (ITU-R BT.601)
 * Scale down for higher quality (quality > 50), up for lower quality
 */
const uint8_t jpeg_quant_table_y[64] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
};

/**
 * Standard chrominance quantization table (ITU-R BT.601)
 */
const uint8_t jpeg_quant_table_c[64] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
};

/* ============================================================================
 * Zigzag Scan Order
 * ========================================================================= */

/**
 * Zigzag scan order for 8x8 block
 * Maps zigzag position to coefficient index
 */
const uint8_t jpeg_zigzag[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* ============================================================================
 * Huffman Table Raw Data
 * ========================================================================= */

/**
 * Huffman DC luminance bits (count of codes per length 1-16)
 */
const uint8_t huff_dc_y_bits[16] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};

/**
 * Huffman DC luminance values (variable-length codes)
 */
const int8_t huff_dc_y_vals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

/**
 * Huffman DC chrominance bits
 */
const uint8_t huff_dc_c_bits[16] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
};

/**
 * Huffman DC chrominance values
 */
const int8_t huff_dc_c_vals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

/**
 * Huffman AC luminance bits
 */
const uint8_t huff_ac_y_bits[16] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125
};

/**
 * Huffman AC luminance values
 * Format: (run_length << 4) | category
 */
const uint8_t huff_ac_y_vals[162] = {
    0x01, 0x02, 0x03, 0x00, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

/**
 * Huffman AC chrominance bits
 */
const uint8_t huff_ac_c_bits[16] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 117
};

/**
 * Huffman AC chrominance values
 * Format: (run_length << 4) | category
 */
const uint8_t huff_ac_c_vals[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

/* ============================================================================
 * Huffman Lookup Table Builder
 * ========================================================================= */

/**
 * Build Huffman lookup table from raw bits/values arrays
 *
 * Decodes the variable-length Huffman codes into a flat lookup table
 * for O(1) encoding. Each symbol gets an entry with its code and length.
 *
 * @param bits  Count of codes per length (16 entries, lengths 1-16)
 * @param vals  Symbol values (variable length)
 * @param table Output lookup table (must be pre-allocated)
 * @param size  Number of entries in the output table
 */
void jpeg_build_huffman_lookup(
    const uint8_t *bits,
    const void *vals,
    huff_entry_t *table,
    uint16_t size
)
{
    uint16_t code = 0;
    uint16_t idx = 0;
    int i;

    /* Clear the output table */
    for (i = 0; i < size; i++) {
        table[i].symbol = 0;
        table[i].code_len = 0;
        table[i].code = 0;
    }

    /* Process lengths 1 through 16 */
    for (i = 0; i < 16; i++) {
        uint8_t count = bits[i];
        uint8_t len = (uint8_t)(i + 1);
        uint16_t j;

        for (j = 0; j < count; j++) {
            uint16_t val;
            uint16_t sym_idx;

            /* Get the symbol value */
            if (size <= 12) {
                /* DC table: int8_t values */
                val = (uint16_t)((const int8_t *)vals)[idx];
            } else {
                /* AC table: uint8_t values */
                val = ((const uint8_t *)vals)[idx];
            }

            idx++;
            sym_idx = val;

            /* Only fill if symbol index is within table bounds */
            if (sym_idx < size) {
                table[sym_idx].symbol = (uint8_t)val;
                table[sym_idx].code_len = len;
                /* Left-aligned code: shift to MSB position */
                table[sym_idx].code = (uint16_t)(code << (16 - len));
            }

            /* Increment code for next symbol at this length */
            code++;
        }

        /* Shift code back for next length's bit positions */
        code <<= 1;
    }
}

/* ============================================================================
 * Pre-decoded Huffman Lookup Tables
 * ========================================================================= */

/**
 * DC luminance Huffman lookup (12 entries for categories 0-11)
 */
const huff_entry_t huff_dc_y[12] = {
    {0,  2, 0x0000},  /* category 0: 00 */
    {1,  3, 0x2000},  /* category 1: 010 */
    {2,  3, 0x3000},  /* category 2: 011 */
    {3,  3, 0x4000},  /* category 3: 100 */
    {4,  3, 0x5000},  /* category 4: 101 */
    {5,  3, 0x6000},  /* category 5: 110 */
    {6,  4, 0x7000},  /* category 6: 1110 */
    {7,  5, 0xf800},  /* category 7: 11110 */
    {8,  6, 0xf800}, /* category 8: 111110 */
    {9,  7, 0xfc00},/* category 9: 1111110 */
    {10, 8, 0xfe00},/* category 10: 11111110 */
    {11, 9, 0xff00} /* category 11: 111111110 */
};

/**
 * DC chrominance Huffman lookup
 */
const huff_entry_t huff_dc_c[12] = {
    {0,  2, 0x0000},  /* category 0: 00 */
    {1,  2, 0x4000},  /* category 1: 01 */
    {2,  2, 0x8000},  /* category 2: 10 */
    {3,  3, 0xc000},  /* category 3: 110 */
    {4,  4, 0xe000},  /* category 4: 1110 */
    {5,  5, 0xf000},  /* category 5: 11110 */
    {6,  6, 0xf000}, /* category 6: 111110 */
    {7,  7, 0xfe00}, /* category 7: 1111110 */
    {8,  8, 0xff00}, /* category 8: 11111110 */
    {9,  9, 0xff80}, /* category 9: 111111110 */
    {10, 10, 0xffc0},/* category 10: 1111111110 */
    {11, 11, 0xffe0} /* category 11: 11111111110 */
};

/**
 * AC luminance Huffman lookup (256 entries)
 * Indexed by (run << 4) | size for O(1) lookup
 *
 * Note: Only 162 entries are used (matching huff_ac_y_vals size)
 * Unused entries are initialized to zeros.
 * Build time: Initialized by jpeg_init_huffman_tables() at encoder init.
 */
const huff_entry_t huff_ac_y[256] = {0};

/**
 * AC chrominance Huffman lookup
 * Build time: Initialized by jpeg_init_huffman_tables() at encoder init.
 */
const huff_entry_t huff_ac_c[256] = {0};

/**
 * Initialize Huffman lookup tables
 * Call this after encoder init to populate AC tables.
 */
void jpeg_init_huffman_tables(void)
{
    /* Build AC luminance table */
    jpeg_build_huffman_lookup(huff_ac_y_bits, huff_ac_y_vals, (huff_entry_t *)huff_ac_y, 256);
    /* Build AC chrominance table */
    jpeg_build_huffman_lookup(huff_ac_c_bits, huff_ac_c_vals, (huff_entry_t *)huff_ac_c, 256);
}
