/**
 * @file jpeg_huffman.c
 * @brief Huffman encoding and bitstream output
 *
 * Implements Huffman encoding for DC and AC coefficients using
 * pre-decoded lookup tables for O(1) encoding, and bitstream
 * output with byte stuffing.
 */

#include "jpeg_encoder.h"
#include "jpeg_tables.h"

/* ============================================================================
 * Bitstream Output Buffer
 * ========================================================================= */

/**
 * Bitstream state structure
 */
typedef struct {
    uint8_t *buffer;      /**< Output buffer pointer */
    uint32_t buf_size;    /**< Buffer size in bytes */
    uint32_t buf_pos;     /**< Current position in buffer */
    uint32_t bit_buffer;  /**< Accumulated bits (up to 24) */
    uint8_t bits_in_buf;  /**< Number of valid bits in buffer */
} bitstream_t;

/**
 * Context for Huffman encoding operations
 */
typedef struct {
    bitstream_t bs;
} huff_context_t;

/* ============================================================================
 * Bitstream Operations
 * ========================================================================= */

/**
 * Initialize bitstream for output
 *
 * @param bs       Bitstream state
 * @param buffer   Output buffer
 * @param buf_size Buffer size in bytes
 */
static void bitstream_init(bitstream_t *bs, uint8_t *buffer, uint32_t buf_size)
{
    bs->buffer = buffer;
    bs->buf_size = buf_size;
    bs->buf_pos = 0;
    bs->bit_buffer = 0;
    bs->bits_in_buf = 0;
}

/**
 * Write bits to bitstream
 *
 * Accumulates bits in a 32-bit buffer and flushes bytes when
 * 16 or more bits are accumulated. Handles 0xFF byte stuffing
 * (JPEG spec requires 0xFF followed by 0x00 to avoid confusion
 * with markers).
 *
 * @param bs    Bitstream state
 * @param bits  Bits to write (interpreted as unsigned)
 * @param len   Number of bits to write (1-16)
 * @return 0 on success, -1 on buffer overflow
 */
static int bitstream_write(bitstream_t *bs, uint16_t bits, uint8_t len)
{
    /* Add bits to buffer (bits are added MSB first) */
    bs->bit_buffer = (bs->bit_buffer << len) | (bits & ((1U << len) - 1));
    bs->bits_in_buf += len;

    /* Flush bytes when we have 16 or more bits */
    while (bs->bits_in_buf >= 8) {
        uint8_t byte;

        /* Extract MSB byte */
        byte = (uint8_t)(bs->bit_buffer >> (bs->bits_in_buf - 8));
        bs->bits_in_buf -= 8;

        /* Check for buffer overflow */
        if (bs->buf_pos >= bs->buf_size) {
            return -1;
        }

        /* Write byte to output */
        bs->buffer[bs->buf_pos++] = byte;

        /* Byte stuffing: 0xFF requires 0x00 following */
        if (byte == 0xFF) {
            if (bs->buf_pos >= bs->buf_size) {
                return -1;
            }
            bs->buffer[bs->buf_pos++] = 0x00;
        }
    }

    return 0;
}

/**
 * Flush remaining bits in bitstream
 *
 * Writes any remaining bits in the buffer, padding with 1s
 * as required by JPEG spec.
 *
 * @param bs    Bitstream state
 * @return 0 on success, -1 on buffer overflow
 */
static int bitstream_flush(bitstream_t *bs)
{
    if (bs->bits_in_buf > 0) {
        uint8_t byte;
        uint8_t pad_bits;

        /* Pad with 1s to fill byte (JPEG spec) */
        pad_bits = 8 - bs->bits_in_buf;
        bs->bit_buffer = (bs->bit_buffer << pad_bits) | ((1U << pad_bits) - 1);
        bs->bits_in_buf = 8;

        /* Extract byte */
        byte = (uint8_t)(bs->bit_buffer >> (bs->bits_in_buf - 8));
        bs->bits_in_buf -= 8;

        /* Check for buffer overflow */
        if (bs->buf_pos >= bs->buf_size) {
            return -1;
        }

        /* Write byte */
        bs->buffer[bs->buf_pos++] = byte;

        /* Byte stuffing for 0xFF */
        if (byte == 0xFF) {
            if (bs->buf_pos >= bs->buf_size) {
                return -1;
            }
            bs->buffer[bs->buf_pos++] = 0x00;
        }
    }

    return 0;
}

/**
 * Get number of bytes written to bitstream
 *
 * @param bs    Bitstream state
 * @return Number of bytes written
 */
static uint32_t bitstream_bytes_written(const bitstream_t *bs)
{
    uint32_t bytes = bs->buf_pos;

    /* Account for any partial byte still in buffer */
    if (bs->bits_in_buf > 0) {
        bytes += 1;
    }

    return bytes;
}

/* ============================================================================
 * Huffman DC Encoding
 * ========================================================================= */

/**
 * Find the category (bit length) for a DC coefficient difference
 *
 * Category is the number of bits needed to represent the magnitude
 * of the difference value:
 *   - Category 0: difference = 0
 *   - Category 1: difference = -1 or +1
 *   - Category 2: difference = -3,-2 or +2,+3
 *   - etc.
 *
 * @param diff  DC coefficient difference (this - prev)
 * @return Category (0-11 for baseline JPEG)
 */
static uint8_t find_dc_category(int16_t diff)
{
    uint8_t category;

    if (diff < 0) {
        /* For negative values, find bit length of (diff - 1) */
        diff = -diff - 1;
    }

    /* Count bits needed */
    category = 0;
    while (diff > 0) {
        category++;
        diff >>= 1;
    }

    return category;
}

/**
 * Encode DC coefficient using Huffman coding
 *
 * Uses O(1) lookup from pre-decoded Huffman table.
 *
 * @param diff      DC coefficient difference (current - previous)
 * @param huff_table Huffman table (huff_dc_y or huff_dc_c)
 * @param bs        Bitstream state
 * @return 0 on success, -1 on error
 */
int jpeg_huff_encode_dc(
    int16_t diff,
    const huff_entry_t *huff_table,
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer)
{
    uint8_t category;
    const huff_entry_t *entry;
    uint16_t bits_to_write;

    /* Find category (0-11) */
    category = find_dc_category(diff);

    /* Look up Huffman code */
    entry = &huff_table[category];

    /* Encode the Huffman code */
    bits_to_write = entry->code;
    *bit_buffer = (*bit_buffer << entry->code_len) | bits_to_write;
    *bits_in_buffer += entry->code_len;

    /* Encode the supplementary bits if category > 0 */
    if (category > 0) {
        uint16_t supplement;
        int16_t supplement_val;

        /* Convert diff to supplementary bits
         * If diff is positive, use raw binary
         * If diff is negative, use (diff - 1) in binary */
        if (diff >= 0) {
            supplement_val = diff;
        } else {
            supplement_val = diff + 1;
        }

        /* Extract the low 'category' bits */
        supplement = (uint16_t)(supplement_val & ((1 << category) - 1));

        *bit_buffer = (*bit_buffer << category) | supplement;
        *bits_in_buffer += category;
    }

    return 0;
}

/* ============================================================================
 * Huffman AC Encoding
 * ========================================================================= */

/**
 * Encode AC coefficients using Huffman coding
 *
 * Uses run-length encoding followed by Huffman coding:
 * - (run << 4) | category forms the symbol
 * - EOB (all zeros remaining) is a special symbol
 *
 * @param block     Input: 63 AC coefficients (zigzag order, skip DC at index 0)
 * @param huff_table Huffman table (huff_ac_y or huff_ac_c)
 * @param bit_buffer Bit buffer for output
 * @param bits_in_buffer Bits in buffer counter
 * @return 0 on success, -1 on error
 */
int jpeg_huff_encode_ac(
    const int16_t *block,
    const huff_entry_t *huff_table,
    uint32_t *bit_buffer,
    uint8_t *bits_in_buffer)
{
    uint8_t i;
    uint8_t run;
    int16_t coeff;
    uint8_t category;
    const huff_entry_t *entry;
    uint16_t bits_to_write;

    run = 0;

    /* Process AC coefficients (indices 1-63) */
    for (i = 1; i < 64; i++) {
        coeff = block[i];

        if (coeff == 0) {
            /* Zero coefficient, increment run */
            run++;
        } else {
            /* Non-zero coefficient */

            /* Handle run overflow (15 consecutive zeros)
             * JPEG allows max run of 15 before a category must be encoded */
            while (run > 15) {
                /* Encode (15, 0) which is the ZRL (zero run length) symbol */
                entry = &huff_table[0xF0];  /* (15 << 4) | 0 = 0xF0 */
                bits_to_write = entry->code;
                *bit_buffer = (*bit_buffer << entry->code_len) | bits_to_write;
                *bits_in_buffer += entry->code_len;

                run -= 16;
            }

            /* Find category */
            if (coeff > 0) {
                category = 0;
                while (coeff > 0) {
                    category++;
                    coeff >>= 1;
                }
            } else {
                /* For negative, find bit length of (coeff - 1) */
                coeff = -coeff - 1;
                category = 0;
                while (coeff > 0) {
                    category++;
                    coeff >>= 1;
                }
            }

            /* Encode symbol: (run << 4) | category */
            entry = &huff_table[(run << 4) | category];
            bits_to_write = entry->code;
            *bit_buffer = (*bit_buffer << entry->code_len) | bits_to_write;
            *bits_in_buffer += entry->code_len;

            /* Encode supplementary bits */
            {
                uint16_t supplement;
                int16_t supp_val;

                /* Get original coefficient value */
                supp_val = block[i];

                /* Convert to supplementary bits */
                if (supp_val >= 0) {
                    supplement = (uint16_t)(supp_val & ((1 << category) - 1));
                } else {
                    supplement = (uint16_t)((supp_val + 1) & ((1 << category) - 1));
                }

                *bit_buffer = (*bit_buffer << category) | supplement;
                *bits_in_buffer += category;
            }

            /* Reset run for next non-zero */
            run = 0;
        }
    }

    /* End of Block (EOB): if run > 0, all remaining ACs are zero */
    if (run > 0) {
        /* Encode EOB: (0, 0) symbol */
        entry = &huff_table[0x00];  /* (0 << 4) | 0 = 0 */
        bits_to_write = entry->code;
        *bit_buffer = (*bit_buffer << entry->code_len) | bits_to_write;
        *bits_in_buffer += entry->code_len;
    }

    return 0;
}

/* ============================================================================
 * Bitstream Interface Functions
 * ========================================================================= */

/**
 * Write bits to output bitstream with byte stuffing
 *
 * High-level interface for direct bitstream writing.
 *
 * @param bits   Bits to write
 * @param len    Number of bits (1-16)
 * @param bs     Bitstream state
 * @return 0 on success, -1 on error
 */
int jpeg_bitstream_write(uint16_t bits, uint8_t len, void *bs_ctx)
{
    huff_context_t *ctx = (huff_context_t *)bs_ctx;

    return bitstream_write(&ctx->bs, bits, len);
}

/**
 * Flush remaining bits to bitstream
 *
 * @param bs_ctx  Bitstream context
 * @return 0 on success, -1 on error
 */
int jpeg_bitstream_flush(void *bs_ctx)
{
    huff_context_t *ctx = (huff_context_t *)bs_ctx;

    return bitstream_flush(&ctx->bs);
}

/**
 * Get bitstream position
 *
 * @param bs_ctx  Bitstream context
 * @return Number of bytes written
 */
uint32_t jpeg_bitstream_get_pos(void *bs_ctx)
{
    huff_context_t *ctx = (huff_context_t *)bs_ctx;

    return bitstream_bytes_written(&ctx->bs);
}
