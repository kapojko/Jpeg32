/**
 * @file jpeg_header.c
 * @brief JPEG header construction for baseline encoding
 *
 * Writes all required JPEG markers in proper order: SOI, APP0, DQT, SOF0,
 * DHT, and SOS. Handles both color (YUV420) and grayscale modes.
 */

#include <stdint.h>
#include <stddef.h>
#include "jpeg_encoder.h"
#include "jpeg_tables.h"

/* ============================================================================
 * JPEG Marker Constants
 * ========================================================================= */

#define MARKER_SOI  0xFFD8
#define MARKER_APP0 0xFFE0
#define MARKER_DQT  0xFFDB
#define MARKER_SOF0 0xFFC0
#define MARKER_DHT  0xFFC4
#define MARKER_SOS  0xFFDA
#define MARKER_EOI  0xFFD9

/* ============================================================================
 * Helper Macros
 * ========================================================================= */

/**
 * Write 16-bit big-endian value to output buffer
 */
static inline void write_be16(uint8_t **buf, uint16_t val)
{
    (*buf)[0] = (uint8_t)(val >> 8);
    (*buf)[1] = (uint8_t)(val & 0xFF);
    *buf += 2;
}

/**
 * Write 8-bit value to output buffer
 */
static inline void write_byte(uint8_t **buf, uint8_t val)
{
    (*buf)[0] = val;
    *buf += 1;
}

/* ============================================================================
 * JPEG Marker Functions
 * ========================================================================= */

/**
 * Write Start of Image marker (SOI)
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_soi(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];

    out[0] = 0xFF;
    out[1] = 0xD8;

    enc->output_bytes += 2;
}

/**
 * Write JFIF APP0 marker segment
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_app0(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];
    uint16_t length = 16;

    /* Marker */
    write_be16(&out, MARKER_APP0);

    /* Length */
    write_be16(&out, length);

    /* Identifier: "JFIF\0" */
    out[0] = 'J';
    out[1] = 'F';
    out[2] = 'I';
    out[3] = 'F';
    out[4] = 0;
    out += 5;

    /* Version: 1.1 */
    out[0] = 1;
    out[1] = 1;
    out += 2;

    /* Units: 0 = no units, aspect ratio only */
    write_byte(&out, 0);

    /* Xdensity: 1 */
    write_be16(&out, 1);

    /* Ydensity: 1 */
    write_be16(&out, 1);

    /* Thumbnail width: 0 */
    write_byte(&out, 0);

    /* Thumbnail height: 0 */
    write_byte(&out, 0);

    enc->output_bytes += length + 2; /* +2 for marker */
}

/**
 * Write Quantization Tables (DQT) marker segment
 *
 * Writes both luminance (table 0) and chrominance (table 1) tables.
 * The tables are scaled by the encoder's quant_recip values for the
 * configured quality setting.
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_dqt(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];
    uint16_t length = 132; /* 2 + 65 + 65 = length + table0 + table1 */
    uint8_t tq0 = 0; /* Table 0: luminance */
    uint8_t tq1 = 1; /* Table 1: chrominance */
    uint16_t i;

    /* Marker */
    write_be16(&out, MARKER_DQT);

    /* Length */
    write_be16(&out, length);

    /* Table 0: luminance (Pq=8, Tq=0) */
    write_byte(&out, tq0);
    for (i = 0; i < 64; i++) {
        write_byte(&out, jpeg_quant_table_y[jpeg_zigzag[i]]);
    }

    /* Table 1: chrominance (Pq=8, Tq=1) */
    write_byte(&out, tq1);
    for (i = 0; i < 64; i++) {
        write_byte(&out, jpeg_quant_table_c[jpeg_zigzag[i]]);
    }

    enc->output_bytes += length + 2; /* +2 for marker */
}

/**
 * Write Start of Frame (SOF0) marker segment
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_sof0(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];
    uint16_t length;
    uint8_t num_components;
    bool is_gray = (enc->config.subsample_mode == JPEG_SUBSAMPLE_GRAY);

    /* Length: 8 for SOF header + 3 per component */
    num_components = is_gray ? 1 : 3;
    length = 8 + (3 * num_components);

    /* Marker */
    write_be16(&out, MARKER_SOF0);

    /* Length */
    write_be16(&out, length);

    /* Precision: 8 bits */
    write_byte(&out, 8);

    /* Image height */
    write_be16(&out, enc->config.height);

    /* Image width */
    write_be16(&out, enc->config.width);

    /* Number of components */
    write_byte(&out, num_components);

    if (is_gray) {
        /* Grayscale: only Y component (id=1, 0x11) */
        write_byte(&out, 1);  /* component id: Y */
        write_byte(&out, 0x11); /* h=1, v=1, Tq=0 */
    } else {
        /* YUV420: Y (id=1, 0x22), Cb (id=2, 0x11), Cr (id=3, 0x11) */
        /* Component 1: Y */
        write_byte(&out, 1);  /* component id: Y */
        write_byte(&out, 0x22); /* h=2, v=2, Tq=0 */
        /* Component 2: Cb */
        write_byte(&out, 2);  /* component id: Cb */
        write_byte(&out, 0x11); /* h=1, v=1, Tq=1 */
        /* Component 3: Cr */
        write_byte(&out, 3);  /* component id: Cr */
        write_byte(&out, 0x11); /* h=1, v=1, Tq=1 */
    }

    enc->output_bytes += length + 2; /* +2 for marker */
}

/**
 * Calculate number of values in a Huffman table from bits array
 *
 * @param bits Array of 16 length counts
 * @return Total number of codes (sum of bits[])
 */
static uint16_t huff_table_val_count(const uint8_t *bits)
{
    uint16_t count = 0;
    uint8_t i;
    for (i = 0; i < 16; i++) {
        count += bits[i];
    }
    return count;
}

/**
 * Write single Huffman table to output buffer
 *
 * @param out    Output buffer pointer (updated)
 * @param tc     Table class (0=DC, 1=AC)
 * @param th     Table id (0 or 1)
 * @param bits   Huffman bits array (16 entries)
 * @param vals   Huffman values array
 * @param nvals  Number of values
 */
static void write_dht_table(
    uint8_t **out,
    uint8_t tc,
    uint8_t th,
    const uint8_t *bits,
    const void *vals,
    uint16_t nvals
)
{
    uint16_t i;

    /* Table class and id */
    write_byte(out, (tc << 4) | th);

    /* Bits counts (16 values) */
    for (i = 0; i < 16; i++) {
        write_byte(out, bits[i]);
    }

    /* Huffman values */
    if (tc == 0) {
        /* DC table: int8_t values */
        const int8_t *dc_vals = (const int8_t *)vals;
        for (i = 0; i < nvals; i++) {
            write_byte(out, (uint8_t)dc_vals[i]);
        }
    } else {
        /* AC table: uint8_t values */
        const uint8_t *ac_vals = (const uint8_t *)vals;
        for (i = 0; i < nvals; i++) {
            write_byte(out, ac_vals[i]);
        }
    }
}

/**
 * Write Huffman Tables (DHT) marker segment
 *
 * Writes four tables: DC luminance, AC luminance, DC chrominance, AC chrominance.
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_dht(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];
    uint16_t length;
    uint16_t dc_y_count, ac_y_count, dc_c_count, ac_c_count;

    /* Calculate value counts */
    dc_y_count = huff_table_val_count(huff_dc_y_bits);
    ac_y_count = huff_table_val_count(huff_ac_y_bits);
    dc_c_count = huff_table_val_count(huff_dc_c_bits);
    ac_c_count = huff_table_val_count(huff_ac_c_bits);

    /* Length = 2 (marker) + 2 (length) + 4 * (1 + 16 + values) */
    length = 2 + 2 + (1 + 16 + dc_y_count) +
             (1 + 16 + ac_y_count) +
             (1 + 16 + dc_c_count) +
             (1 + 16 + ac_c_count);

    /* Marker */
    write_be16(&out, MARKER_DHT);

    /* Length */
    write_be16(&out, length);

    /* DC luminance (class=0, id=0) */
    write_dht_table(&out, 0, 0, huff_dc_y_bits, huff_dc_y_vals, dc_y_count);

    /* AC luminance (class=1, id=0) */
    write_dht_table(&out, 1, 0, huff_ac_y_bits, huff_ac_y_vals, ac_y_count);

    /* DC chrominance (class=0, id=1) */
    write_dht_table(&out, 0, 1, huff_dc_c_bits, huff_dc_c_vals, dc_c_count);

    /* AC chrominance (class=1, id=1) */
    write_dht_table(&out, 1, 1, huff_ac_c_bits, huff_ac_c_vals, ac_c_count);

    enc->output_bytes += length + 2; /* +2 for marker */
}

/**
 * Write Start of Scan (SOS) marker segment
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_sos(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];
    uint16_t length;
    uint8_t num_components;
    bool is_gray = (enc->config.subsample_mode == JPEG_SUBSAMPLE_GRAY);

    /* Length: 6 for SOS header + 2 per component + 3 */
    num_components = is_gray ? 1 : 3;
    length = 6 + (2 * num_components) + 3;

    /* Marker */
    write_be16(&out, MARKER_SOS);

    /* Length */
    write_be16(&out, length);

    /* Number of components */
    write_byte(&out, num_components);

    if (is_gray) {
        /* Grayscale: Y component (id=1, DC table 0, AC table 0) */
        write_byte(&out, 1);  /* component id: Y */
        write_byte(&out, 0x00); /* DC table 0, AC table 0 */
    } else {
        /* YUV420: Y (id=1, 0x00), Cb (id=2, 0x11), Cr (id=3, 0x11) */
        /* Component 1: Y */
        write_byte(&out, 1);  /* component id: Y */
        write_byte(&out, 0x00); /* DC table 0, AC table 0 */
        /* Component 2: Cb */
        write_byte(&out, 2);  /* component id: Cb */
        write_byte(&out, 0x11); /* DC table 1, AC table 1 */
        /* Component 3: Cr */
        write_byte(&out, 3);  /* component id: Cr */
        write_byte(&out, 0x11); /* DC table 1, AC table 1 */
    }

    /* Start of spectral selection */
    write_byte(&out, 0);

    /* End of spectral selection */
    write_byte(&out, 63);

    /* Successive approximation */
    write_byte(&out, 0);

    enc->output_bytes += length + 2; /* +2 for marker */
}

/**
 * Write End of Image marker (EOI)
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 */
void jpeg_write_eoi(jpeg_encoder_t *enc)
{
    uint8_t *out = &enc->config.output_buffer[enc->output_bytes];

    out[0] = 0xFF;
    out[1] = 0xD9;

    enc->output_bytes += 2;
}

/**
 * Build complete JPEG header
 *
 * Writes all required markers in proper order:
 * SOI → APP0 → DQT → SOF0 → DHT → SOS
 *
 * @param enc Encoder state (output_buffer and output_bytes updated)
 * @return Total header size in bytes (excluding EOI)
 */
uint32_t jpeg_build_header(jpeg_encoder_t *enc)
{
    uint32_t start_bytes = enc->output_bytes;

    /* Write markers in JPEG specified order */
    jpeg_write_soi(enc);
    jpeg_write_app0(enc);
    jpeg_write_dqt(enc);
    jpeg_write_sof0(enc);
    jpeg_write_dht(enc);
    jpeg_write_sos(enc);

    return enc->output_bytes - start_bytes;
}
