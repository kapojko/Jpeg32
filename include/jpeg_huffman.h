/**
 * @file jpeg_huffman.h
 * @brief Huffman encoding for JPEG entropy coding
 *
 * Provides DC/AC Huffman encoding with O(1) lookup and
 * bitstream output with byte stuffing.
 */
#ifndef JPEG_HUFFMAN_H
#define JPEG_HUFFMAN_H

#include <stdint.h>
#include "jpeg_tables.h"

/**
 * Encode DC coefficient difference
 *
 * @param diff DC coefficient difference (current - previous)
 * @param huff_table Huffman table (huff_dc_y or huff_dc_c)
 * @param bit_buffer Bit buffer for output
 * @param bits_in_buffer Bits in buffer counter
 * @return 0 on success, -1 on error
 */
int jpeg_huff_encode_dc(
    int16_t diff,
    const huff_entry_t *huff_table,
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer
);

/**
 * Encode AC coefficients using Huffman coding
 *
 * Uses run-length encoding followed by Huffman coding:
 * - (run << 4) | category forms the symbol
 * - EOB (all zeros remaining) is a special symbol
 *
 * @param block Input: 63 AC coefficients (zigzag order, skip DC at index 0)
 * @param huff_table Huffman table (huff_ac_y or huff_ac_c)
 * @param bit_buffer Bit buffer for output
 * @param bits_in_buffer Bits in buffer counter
 * @return 0 on success, -1 on error
 */
int jpeg_huff_encode_ac(
    const int16_t *block,
    const huff_entry_t *huff_table,
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer
);

/**
 * Write bits to output buffer
 *
 * @param bits Bits to write
 * @param len Number of bits (1-16)
 * @param bit_buffer Bit buffer
 * @param bits_in_buffer Bits in buffer counter
 * @param output Output buffer
 * @param output_size Output buffer size
 * @param output_pos Current output position (updated)
 * @return 0 on success, -1 on error
 */
int jpeg_bitstream_write(
    uint16_t bits,
    uint8_t len,
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer,
    uint8_t *output,
    uint32_t output_size,
    uint32_t *output_pos
);

/**
 * Flush remaining bits in buffer
 *
 * @param bit_buffer Bit buffer
 * @param bits_in_buffer Bits in buffer counter
 * @param output Output buffer
 * @param output_size Output buffer size
 * @param output_pos Current output position (updated)
 * @return 0 on success, -1 on error
 */
int jpeg_bitstream_flush(
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer,
    uint8_t *output,
    uint32_t output_size,
    uint32_t *output_pos
);

#endif /* JPEG_HUFFMAN_H */
