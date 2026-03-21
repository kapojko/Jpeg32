/**
 * @file jpeg_dct.c
 * @brief 8x8 DCT (Discrete Cosine Transform) implementation
 *
 * Implements forward DCT using the AAN (Arai-Agui-Nakajima) algorithm
 * with fixed-point approximation for embedded systems.
 */

#include "jpeg_encoder.h"
#include "jpeg_config.h"

/* ============================================================================
 * AAN DCT Fixed-Point Constants
 * ========================================================================= */

/**
 * AAN DCT scaling factors (approximated as fractions)
 * These are derived from the AAN algorithm's pre-computed constants:
 *
 * a0 = 0.707106781 (cos(45°))  -> 181/256
 * a1 = 0.382683432 (cos(22.5°)) -> 98/256
 * a2 = 0.541196100 (cos(67.5°)) -> 138/256
 * a3 = 1.306562965 (sqrt(2)*cos(22.5°)) -> 334/256
 */

/**
 * a0 = cos(pi/4) = 1/sqrt(2) ≈ 0.7071
 * Fixed-point: 0.7071 * 256 ≈ 181
 */
#define AAN_A0  181

/**
 * a1 = cos(pi/8) ≈ 0.3827
 * Fixed-point: 0.3827 * 256 ≈ 98
 */
#define AAN_A1  98

/**
 * a2 = cos(3pi/8) ≈ 0.5412
 * Fixed-point: 0.5412 * 256 ≈ 138
 */
#define AAN_A2  138

/**
 * a3 = sqrt(2) * cos(pi/8) ≈ 1.3066
 * Fixed-point: 1.3066 * 256 ≈ 334
 */
#define AAN_A3  334

/**
 * Number of fractional bits for DCT intermediate calculations
 */
#define DCT_FRAC_BITS  12

/**
 * Scale factor for DCT output (accounts for fixed-point scaling)
 */
#define DCT_SCALE      (1 << DCT_FRAC_BITS)

/* ============================================================================
 * Row DCT Constants (butterfly operations)
 * ========================================================================= */

/**
 * Row DCT temporary buffer size
 */
#define ROW_DCT_SIZE   8

/* ============================================================================
 * Reference C Implementation (Two-Pass DCT)
 * ========================================================================= */

/**
 * Perform 1-D DCT on a single row (in-place)
 *
 * Uses the AAN algorithm with pre-scaling:
 * - First applies pre-rotation stage
 * - Then performs butterfly operations
 * - Output scaled by 1/8
 *
 * @param row    Input/Output: 8-element array (int32_t for intermediates)
 */
static void dct_row_1d(int32_t *row)
{
    int32_t x0, x1, x2, x3, x4, x5, x6, x7;
    int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int32_t z1, z2, z3, z4, z5;
    int32_t v0, v1, v2, v3, v4, v5, v6, v7;

    /* Input with level shift: subtract 128 to center around 0 */
    x0 = row[0];
    x1 = row[1];
    x2 = row[2];
    x3 = row[3];
    x4 = row[4];
    x5 = row[5];
    x6 = row[6];
    x7 = row[7];

    /* Even phase - "butterfly" operations */
    tmp0 = x0 + x7;
    tmp1 = x1 + x6;
    tmp2 = x2 + x5;
    tmp3 = x3 + x4;

    /* Odd phase - pre-rotation */
    tmp4 = x0 - x7;
    tmp5 = x1 - x6;
    tmp6 = x2 - x5;
    tmp7 = x3 - x4;

    /* Even outputs: even-indexed frequency components */
    v0 = tmp0 + tmp3;
    v1 = tmp1 + tmp2;
    v4 = tmp0 - tmp3;
    v5 = tmp1 - tmp2;

    /* Odd outputs: odd-indexed frequency components */
    /* Apply AAN scaling for odd terms */
    z1 = (tmp4 + tmp7) * AAN_A1;
    z2 = tmp5 * AAN_A2 + tmp6 * AAN_A3;
    z3 = tmp4 * AAN_A3 - tmp6 * AAN_A2;
    z4 = tmp5 * AAN_A1 + tmp7 * AAN_A3;
    z5 = (tmp5 - tmp7) * AAN_A0;

    /* Complete odd terms with additional scaling */
    v2 = (z1 + z4) >> (DCT_FRAC_BITS - 8);
    v3 = (z2 - z3) >> (DCT_FRAC_BITS - 8);
    v6 = (z1 - z4) >> (DCT_FRAC_BITS - 8);
    v7 = (z2 + z3) >> (DCT_FRAC_BITS - 8);

    /* Store even terms (no rotation needed) */
    row[0] = v0;
    row[2] = v4;
    row[4] = v1;
    row[6] = v5;

    /* Store odd terms with rotation */
    row[1] = v2;
    row[3] = v6;
    row[5] = v7;
    row[7] = v3;
}

/**
 * Perform 1-D DCT on a single column (in-place)
 *
 * Same algorithm as row DCT, applied vertically.
 *
 * @param block  Input/Output: 8x8 block (column access via stride)
 * @param col    Column index (0-7)
 */
static void dct_col_1d(int16_t *block, uint8_t col)
{
    int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int32_t v0, v1, v2, v3, v4, v5, v6, v7;
    int32_t z1, z2, z3, z4, z5;
    int32_t row0, row1, row2, row3, row4, row5, row6, row7;
    int32_t result[8];

    /* Read column */
    row0 = block[0 * 8 + col];
    row1 = block[1 * 8 + col];
    row2 = block[2 * 8 + col];
    row3 = block[3 * 8 + col];
    row4 = block[4 * 8 + col];
    row5 = block[5 * 8 + col];
    row6 = block[6 * 8 + col];
    row7 = block[7 * 8 + col];

    /* Even phase */
    tmp0 = row0 + row7;
    tmp1 = row1 + row6;
    tmp2 = row2 + row5;
    tmp3 = row3 + row4;

    tmp4 = row0 - row7;
    tmp5 = row1 - row6;
    tmp6 = row2 - row5;
    tmp7 = row3 - row4;

    /* Even outputs */
    v0 = tmp0 + tmp3;
    v1 = tmp1 + tmp2;
    v4 = tmp0 - tmp3;
    v5 = tmp1 - tmp2;

    /* Odd outputs with AAN scaling */
    z1 = (tmp4 + tmp7) * AAN_A1;
    z2 = tmp5 * AAN_A2 + tmp6 * AAN_A3;
    z3 = tmp4 * AAN_A3 - tmp6 * AAN_A2;
    z4 = tmp5 * AAN_A1 + tmp7 * AAN_A3;
    z5 = (tmp5 - tmp7) * AAN_A0;

    v2 = (z1 + z4) >> (DCT_FRAC_BITS - 8);
    v3 = (z2 - z3) >> (DCT_FRAC_BITS - 8);
    v6 = (z1 - z4) >> (DCT_FRAC_BITS - 8);
    v7 = (z2 + z3) >> (DCT_FRAC_BITS - 8);

    /* Store results */
    result[0] = v0;
    result[1] = v2;
    result[2] = v4;
    result[3] = v6;
    result[4] = v1;
    result[5] = v7;
    result[6] = v5;
    result[7] = v3;

    /* Write column back with final scaling (divide by 8) */
    block[0 * 8 + col] = (int16_t)(result[0] >> 3);
    block[1 * 8 + col] = (int16_t)(result[1] >> 3);
    block[2 * 8 + col] = (int16_t)(result[2] >> 3);
    block[3 * 8 + col] = (int16_t)(result[3] >> 3);
    block[4 * 8 + col] = (int16_t)(result[4] >> 3);
    block[5 * 8 + col] = (int16_t)(result[5] >> 3);
    block[6 * 8 + col] = (int16_t)(result[6] >> 3);
    block[7 * 8 + col] = (int16_t)(result[7] >> 3);
}

/**
 * Reference 8x8 DCT implementation (in-place)
 *
 * Uses AAN algorithm with two-pass approach:
 * 1. Row pass: DCT on each row
 * 2. Column pass: DCT on each column
 *
 * Input should be signed values (level-shifted from [0,255] to [-128,127])
 * Output is 8-bit range but stored as int16_t for headroom.
 *
 * @param block  Input/Output: 8x8 block of signed pixel values
 */
void jpeg_dct8x8_ref(int16_t *block)
{
    uint8_t row;
    int32_t row_data[8];

    /* Row pass: process each row */
    for (row = 0; row < 8; row++) {
        /* Copy row to temporary (need 32-bit for intermediate calculations) */
        row_data[0] = block[row * 8 + 0];
        row_data[1] = block[row * 8 + 1];
        row_data[2] = block[row * 8 + 2];
        row_data[3] = block[row * 8 + 3];
        row_data[4] = block[row * 8 + 4];
        row_data[5] = block[row * 8 + 5];
        row_data[6] = block[row * 8 + 6];
        row_data[7] = block[row * 8 + 7];

        /* Apply 1-D DCT to row */
        dct_row_1d(row_data);

        /* Copy back (truncate to 16-bit, clamp if needed) */
        block[row * 8 + 0] = (int16_t)row_data[0];
        block[row * 8 + 1] = (int16_t)row_data[1];
        block[row * 8 + 2] = (int16_t)row_data[2];
        block[row * 8 + 3] = (int16_t)row_data[3];
        block[row * 8 + 4] = (int16_t)row_data[4];
        block[row * 8 + 5] = (int16_t)row_data[5];
        block[row * 8 + 6] = (int16_t)row_data[6];
        block[row * 8 + 7] = (int16_t)row_data[7];
    }

    /* Column pass: process each column */
    for (row = 0; row < 8; row++) {
        dct_col_1d(block, row);
    }
}

/* ============================================================================
 * ARM DSP Optimized Implementation
 * ========================================================================= */

#if JPEG_USE_ARM_DSP

/**
 * ARM DSP optimized 8x8 DCT implementation
 *
 * Uses ARM DSP intrinsics for improved performance:
 * - __SSAT: Signed saturation
 * - __SMLAD: Signed multiply-accumulate dual (2x32-bit products)
 *
 * The algorithm is the same AAN algorithm but optimized for ARM DSP.
 */
void jpeg_dct8x8_arm(int16_t *block)
{
    /* ARM-specific implementation using DSP intrinsics */
    /* This is a simplified version - full optimization would use
     * inline assembly or more sophisticated intrinsics */

    uint8_t row;
    uint8_t col;
    int32_t row_data[8];
    int32_t col_data[8];

    /* Row pass */
    for (row = 0; row < 8; row++) {
        /* Load row into 32-bit array */
        row_data[0] = block[row * 8 + 0];
        row_data[1] = block[row * 8 + 1];
        row_data[2] = block[row * 8 + 2];
        row_data[3] = block[row * 8 + 3];
        row_data[4] = block[row * 8 + 4];
        row_data[5] = block[row * 8 + 5];
        row_data[6] = block[row * 8 + 6];
        row_data[7] = block[row * 8 + 7];

        /* AAN DCT with ARM optimizations */
        {
            int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
            int32_t z1, z2, z3, z4, z5;
            int32_t v0, v1, v2, v3, v4, v5, v6, v7;

            /* Even phase */
            tmp0 = row_data[0] + row_data[7];
            tmp1 = row_data[1] + row_data[6];
            tmp2 = row_data[2] + row_data[5];
            tmp3 = row_data[3] + row_data[4];

            /* Odd phase */
            tmp4 = row_data[0] - row_data[7];
            tmp5 = row_data[1] - row_data[6];
            tmp6 = row_data[2] - row_data[5];
            tmp7 = row_data[3] - row_data[4];

            /* Even outputs */
            v0 = tmp0 + tmp3;
            v1 = tmp1 + tmp2;
            v4 = tmp0 - tmp3;
            v5 = tmp1 - tmp2;

            /* Odd outputs with AAN scaling - use DSP-friendly operations */
            z1 = __SSAT(((tmp4 + tmp7) * AAN_A1), 16);
            z2 = __SSAT((tmp5 * AAN_A2 + tmp6 * AAN_A3), 18);
            z3 = __SSAT((tmp4 * AAN_A3 - tmp6 * AAN_A2), 18);
            z4 = __SSAT((tmp5 * AAN_A1 + tmp7 * AAN_A3), 16);
            z5 = __SSAT(((tmp5 - tmp7) * AAN_A0), 16);

            v2 = (z1 + z4) >> (8 - DCT_FRAC_BITS);
            v3 = (z2 - z3) >> (8 - DCT_FRAC_BITS);
            v6 = (z1 - z4) >> (8 - DCT_FRAC_BITS);
            v7 = (z2 + z3) >> (8 - DCT_FRAC_BITS);

            row_data[0] = v0;
            row_data[1] = v2;
            row_data[2] = v4;
            row_data[3] = v6;
            row_data[4] = v1;
            row_data[5] = v7;
            row_data[6] = v5;
            row_data[7] = v3;
        }

        /* Copy back */
        block[row * 8 + 0] = (int16_t)__SSAT(row_data[0], 16);
        block[row * 8 + 1] = (int16_t)__SSAT(row_data[1], 16);
        block[row * 8 + 2] = (int16_t)__SSAT(row_data[2], 16);
        block[row * 8 + 3] = (int16_t)__SSAT(row_data[3], 16);
        block[row * 8 + 4] = (int16_t)__SSAT(row_data[4], 16);
        block[row * 8 + 5] = (int16_t)__SSAT(row_data[5], 16);
        block[row * 8 + 6] = (int16_t)__SSAT(row_data[6], 16);
        block[row * 8 + 7] = (int16_t)__SSAT(row_data[7], 16);
    }

    /* Column pass */
    for (col = 0; col < 8; col++) {
        /* Load column */
        col_data[0] = block[0 * 8 + col];
        col_data[1] = block[1 * 8 + col];
        col_data[2] = block[2 * 8 + col];
        col_data[3] = block[3 * 8 + col];
        col_data[4] = block[4 * 8 + col];
        col_data[5] = block[5 * 8 + col];
        col_data[6] = block[6 * 8 + col];
        col_data[7] = block[7 * 8 + col];

        /* AAN DCT column */
        {
            int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
            int32_t z1, z2, z3, z4, z5;
            int32_t v0, v1, v2, v3, v4, v5, v6, v7;

            tmp0 = col_data[0] + col_data[7];
            tmp1 = col_data[1] + col_data[6];
            tmp2 = col_data[2] + col_data[5];
            tmp3 = col_data[3] + col_data[4];

            tmp4 = col_data[0] - col_data[7];
            tmp5 = col_data[1] - col_data[6];
            tmp6 = col_data[2] - col_data[5];
            tmp7 = col_data[3] - col_data[4];

            v0 = tmp0 + tmp3;
            v1 = tmp1 + tmp2;
            v4 = tmp0 - tmp3;
            v5 = tmp1 - tmp2;

            z1 = (tmp4 + tmp7) * AAN_A1;
            z2 = tmp5 * AAN_A2 + tmp6 * AAN_A3;
            z3 = tmp4 * AAN_A3 - tmp6 * AAN_A2;
            z4 = tmp5 * AAN_A1 + tmp7 * AAN_A3;
            z5 = (tmp5 - tmp7) * AAN_A0;

            v2 = (z1 + z4) >> (8 - DCT_FRAC_BITS);
            v3 = (z2 - z3) >> (8 - DCT_FRAC_BITS);
            v6 = (z1 - z4) >> (8 - DCT_FRAC_BITS);
            v7 = (z2 + z3) >> (8 - DCT_FRAC_BITS);

            col_data[0] = v0 >> 3;  /* Final divide by 8 */
            col_data[1] = v2 >> 3;
            col_data[2] = v4 >> 3;
            col_data[3] = v6 >> 3;
            col_data[4] = v1 >> 3;
            col_data[5] = v7 >> 3;
            col_data[6] = v5 >> 3;
            col_data[7] = v3 >> 3;
        }

        /* Write back with saturation */
        block[0 * 8 + col] = (int16_t)__SSAT(col_data[0], 16);
        block[1 * 8 + col] = (int16_t)__SSAT(col_data[1], 16);
        block[2 * 8 + col] = (int16_t)__SSAT(col_data[2], 16);
        block[3 * 8 + col] = (int16_t)__SSAT(col_data[3], 16);
        block[4 * 8 + col] = (int16_t)__SSAT(col_data[4], 16);
        block[5 * 8 + col] = (int16_t)__SSAT(col_data[5], 16);
        block[6 * 8 + col] = (int16_t)__SSAT(col_data[6], 16);
        block[7 * 8 + col] = (int16_t)__SSAT(col_data[7], 16);
    }
}

#endif /* JPEG_USE_ARM_DSP */

/* ============================================================================
 * DCT Dispatcher
 * ========================================================================= */

/**
 * 8x8 DCT dispatcher
 *
 * Automatically selects the appropriate implementation based on
 * platform capabilities (ARM DSP vs reference C).
 *
 * @param block  Input/Output: 8x8 block of signed pixel values
 */
void jpeg_dct8x8(int16_t *block)
{
#if JPEG_USE_ARM_DSP
    jpeg_dct8x8_arm(block);
#else
    jpeg_dct8x8_ref(block);
#endif
}
