/**
 * @file jpeg_header.h
 * @brief JPEG header construction functions
 *
 * Provides functions for writing JPEG markers and headers.
 */
#ifndef JPEG_HEADER_H
#define JPEG_HEADER_H

#include <stdint.h>
#include "jpeg_encoder.h"

/**
 * Write Start of Image marker (SOI: 0xFFD8)
 * @param enc Encoder state
 */
void jpeg_write_soi(jpeg_encoder_t *enc);

/**
 * Write JFIF APP0 marker segment
 * @param enc Encoder state
 */
void jpeg_write_app0(jpeg_encoder_t *enc);

/**
 * Write Quantization Tables (DQT)
 * @param enc Encoder state
 */
void jpeg_write_dqt(jpeg_encoder_t *enc);

/**
 * Write Start of Frame marker (SOF0 - Baseline DCT)
 * @param enc Encoder state
 */
void jpeg_write_sof0(jpeg_encoder_t *enc);

/**
 * Write Huffman Tables (DHT)
 * @param enc Encoder state
 */
void jpeg_write_dht(jpeg_encoder_t *enc);

/**
 * Write Start of Scan marker (SOS)
 * @param enc Encoder state
 */
void jpeg_write_sos(jpeg_encoder_t *enc);

/**
 * Write End of Image marker (EOI: 0xFFD9)
 * @param enc Encoder state
 */
void jpeg_write_eoi(jpeg_encoder_t *enc);

/**
 * Build complete JPEG header (SOI through SOS)
 * @param enc Encoder state
 * @return Total header size in bytes
 */
uint32_t jpeg_build_header(jpeg_encoder_t *enc);

#endif /* JPEG_HEADER_H */
