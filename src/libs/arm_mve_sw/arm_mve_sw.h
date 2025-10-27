/**
 * @file arm_mve_sw.h
 *
 */

#ifndef ARM_MVE_SW_H
#define ARM_MVE_SW_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

typedef struct {
    uint32_t val[4];
} uint32x4_t;

typedef struct {
    uint16_t val[8];
} uint16x8_t;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

static inline uint16x8_t vldrbq_u16(const uint8_t * base)
{
    uint16x8_t ret;
    for(int i = 0; i < 8; i++) {
        ret.val[i] = base[i];
    }
    return ret;
}

static inline uint32x4_t vldrbq_u32(const uint8_t * base)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = base[i];
    }
    return ret;
}

static inline uint16x8_t vmlaq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c)
{
    uint16x8_t ret;
    for(int i = 0; i < 8; i++) {
        ret.val[i] = a.val[i] + b.val[i] * c;
    }
    return ret;
}

static inline uint32x4_t vmlaq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = a.val[i] + b.val[i] * c;
    }
    return ret;
}

static inline uint16x8_t vmulq_n_u16(uint16x8_t a, uint16_t b)
{
    uint16x8_t ret;
    for(int i = 0; i < 8; i++) {
        ret.val[i] = a.val[i] * b;
    }
    return ret;
}

static inline uint32x4_t vmulq_n_u32(uint32x4_t a, uint32_t b)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = a.val[i] * b;
    }
    return ret;
}

static inline uint16x8_t vorrq_u16(uint16x8_t a, uint16x8_t b)
{
    uint16x8_t ret;
    for(int i = 0; i < 8; i++) {
        ret.val[i] = a.val[i] | b.val[i];
    }
    return ret;
}

static inline uint16x8_t vreinterpretq_u16_u32(uint32x4_t a)
{
    uint16x8_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i * 2 + 0] = (uint16_t)(a.val[i] & 0xFFFF);
        ret.val[i * 2 + 1] = (uint16_t)((a.val[i] >> 16) & 0xFFFF);
    }
    return ret;
}

static inline uint32x4_t vreinterpretq_u32_u16(uint16x8_t a)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = (uint32_t)a.val[i * 2] | ((uint32_t)a.val[i * 2 + 1] << 16);
    }
    return ret;
}

static inline uint32x4_t vshlq_n_u32(uint32x4_t a, const int imm)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = a.val[i] << imm;
    }
    return ret;
}

static inline uint16x8_t vshrq_n_u16(uint16x8_t a, const int imm)
{
    uint16x8_t ret;
    for(int i = 0; i < 8; i++) {
        ret.val[i] = a.val[i] >> imm;
    }
    return ret;
}

static inline uint32x4_t vshrq_n_u32(uint32x4_t a, const int imm)
{
    uint32x4_t ret;
    for(int i = 0; i < 4; i++) {
        ret.val[i] = a.val[i] >> imm;
    }
    return ret;
}

static inline void vstrbq_u16(uint8_t * base, uint16x8_t value)
{
    for(int i = 0; i < 8; i++) {
        base[i] = value.val[i];
    }
}

static inline void vstrbq_u32(uint8_t * base, uint32x4_t value)
{
    for(int i = 0; i < 4; i++) {
        base[i] = value.val[i];
    }
}

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*ARM_MVE_SW_H*/
