/**
 * @file jpeg_dct.h
 * @brief 8x8 DCT (Discrete Cosine Transform) implementation
 *
 * Provides forward DCT using AAN (Arai-Agui-Nakajima) algorithm.
 * Reference C implementation for all platforms, ARM DSP optimized
 * version when JPEG_USE_ARM_DSP is defined.
 */
#ifndef JPEG_DCT_H
#define JPEG_DCT_H

#include <stdint.h>

/**
 * Perform 8x8 forward DCT (in-place)
 *
 * Uses AAN algorithm with fixed-point arithmetic.
 * On ARM platforms with DSP extensions, uses optimized intrinsics.
 *
 * @param block Input 8x8 block (int16_t), transformed in-place to DCT coefficients
 */
void jpeg_dct8x8(int16_t *block);

#endif /* JPEG_DCT_H */
