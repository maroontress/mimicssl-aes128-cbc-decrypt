/*
    References:

    https://en.wikipedia.org/wiki/Advanced_Encryption_Standard
    https://en.wikipedia.org/wiki/AES_key_schedule
    https://en.wikipedia.org/wiki/Rijndael_S-box
    https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
*/

/*
    References of ARM Neon Intrinsics:
    https://arm-software.github.io/acle/neon_intrinsics/advsimd.html
    https://developer.arm.com/documentation/dui0472/m/Using-NEON-Support?lang=en
*/

#include <arm_neon.h>
#include "Aes128Cbc.h"

#include "sbox.h"
#include "rcon.h"

static void
keyExpansion(const struct Aes128Cbc_Key *key, struct Aes128Cbc_RoundKey *out)
{
    out->round[0] = *key;
    uint32_t *abcd = (uint32_t *)&(out->round[0].data[12]);
    uint32_t t = *abcd;

    // prev **** **** abcd
    // next
    for (uint32_t round = 1; round < 11; ++round) {
        uint32_t t0 = (t >> 8) | (t << 24);
        uint8_t a0 = (uint8_t)t0;
        uint8_t b0 = (uint8_t)(t0 >> 8);
        uint8_t c0 = (uint8_t)(t0 >> 16);
        uint8_t d0 = (uint8_t)(t0 >> 24);
        t = (SBOX[a0] ^ RCON[round - 1])
            | (SBOX[b0] << 8)
            | (SBOX[c0] << 16)
            | (SBOX[d0] << 24);
        const uint32_t *prev = (const uint32_t *)&out->round[round - 1];
        uint32_t *next = (uint32_t *)&out->round[round];
        for (uint32_t j = 0; j < 4; ++j) {
            t ^= *prev;
            *next = t;
            ++next;
            ++prev;
        }
    }
}

static void
postKeyExpansion(struct Aes128Cbc_RoundKey *roundKey)
{
    for (int k = 1; k < 10; ++k) {
        struct Aes128Cbc_Key *key = &roundKey->round[k];
        uint8x16_t key128 = vaesimcq_u8(vld1q_u8(key->data));
        vst1q_u8(key->data, key128);
    }
}

void
Aes128Cbc_init(struct Aes128Cbc *ctx, const struct Aes128Cbc_Key *key,
    const struct Aes128Cbc_Iv *iv)
{
    keyExpansion(key, &ctx->roundKey);
    postKeyExpansion(&ctx->roundKey);
    ctx->iv = *iv;
}

static uint16x8_t
eqInvCipher(uint16x8_t state, const struct Aes128Cbc_RoundKey *roundKey)
{
    const struct Aes128Cbc_Key *round = roundKey->round;
    for (uint32_t k = 10; k > 1; --k) {
        state = vaesimcq_u8(vaesdq_u8(state, vld1q_u8(round[k].data)));
    }
    state = vaesdq_u8(state, vld1q_u8(round[1].data));
    state = veorq_u8(state, vld1q_u8(round[0].data));
    return state;
}

void Aes128Cbc_decrypt(struct Aes128Cbc *ctx, const void *data,
    size_t length, void *output)
{
    const uint8_t *in = (const uint8_t *)data;
    uint8_t *out = (uint8_t *)output;
    uint8x16_t iv128 = vld1q_u8(ctx->iv.data);
    while (length > 0) {
        uint8x16_t in128 = vld1q_u8(in);
        uint8x16_t state = eqInvCipher(in128, &ctx->roundKey);
        vst1q_u8(out, veorq_u8(state, iv128));
        iv128 = in128;
        in += 16;
        out += 16;
        length -= 16;
    }
    vst1q_u8(ctx->iv.data, iv128);
}
