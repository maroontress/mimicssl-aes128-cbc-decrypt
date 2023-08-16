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
#include "rsbox.h"
#include "rcon.h"
#include "multiply.h"

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

static uint8x16_t
addRoundKey(uint8x16_t state, const struct Aes128Cbc_Key *key)
{
    uint8x16_t r = vld1q_u8(key->data);
    return veorq_u8(state, r);
}

static uint8x16_t
invShiftRowsSubBytes(uint8x16_t state)
{
    uint8_t s[16];
    uint8_t o[16];

    // 0 1 2 3 4 5 6 7 8 9 A B C D E F
    // |       |       |       |
    // 0 D A 7 4 1 E B 8 5 2 F C 9 6 3
    vst1q_u8(s, state);
    o[0] = RSBOX[s[0]];
    o[1] = RSBOX[s[13]];
    o[2] = RSBOX[s[10]];
    o[3] = RSBOX[s[7]];
    o[4] = RSBOX[s[4]];
    o[5] = RSBOX[s[1]];
    o[6] = RSBOX[s[14]];
    o[7] = RSBOX[s[11]];
    o[8] = RSBOX[s[8]];
    o[9] = RSBOX[s[5]];
    o[10] = RSBOX[s[2]];
    o[11] = RSBOX[s[15]];
    o[12] = RSBOX[s[12]];
    o[13] = RSBOX[s[9]];
    o[14] = RSBOX[s[6]];
    o[15] = RSBOX[s[3]];
    return vld1q_u8(o);
}

static uint8x16_t
invMixColumns(uint8x16_t state)
{
    uint8_t s[16];
    uint32_t v[4];

    vst1q_u8(s, state);
    v[0] = MULTIPLY_0[s[0]];
    v[1] = MULTIPLY_0[s[4]];
    v[2] = MULTIPLY_0[s[8]];
    v[3] = MULTIPLY_0[s[12]];
    uint8x16_t a = vld1q_u8((uint8_t *)v);

    v[0] = MULTIPLY_1[s[1]];
    v[1] = MULTIPLY_1[s[5]];
    v[2] = MULTIPLY_1[s[9]];
    v[3] = MULTIPLY_1[s[13]];
    uint8x16_t b = vld1q_u8((uint8_t *)v);

    v[0] = MULTIPLY_2[s[2]];
    v[1] = MULTIPLY_2[s[6]];
    v[2] = MULTIPLY_2[s[10]];
    v[3] = MULTIPLY_2[s[14]];
    uint8x16_t c = vld1q_u8((uint8_t *)v);

    v[0] = MULTIPLY_3[s[3]];
    v[1] = MULTIPLY_3[s[7]];
    v[2] = MULTIPLY_3[s[11]];
    v[3] = MULTIPLY_3[s[15]];
    uint8x16_t d = vld1q_u8((uint8_t *)v);

    return veorq_u8(veorq_u8(a, b), veorq_u8(c, d));
}

static void
postKeyExpansion(struct Aes128Cbc_RoundKey *roundKey)
{
    for (int k = 1; k < 10; ++k) {
        struct Aes128Cbc_Key *key = &roundKey->round[k];
        uint8x16_t o = vld1q_u8(key->data);
        vst1q_u8(key->data, invMixColumns(o));
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

static uint8x16_t
eqInvCipher(uint8x16_t state, const struct Aes128Cbc_RoundKey *roundKey)
{
    const struct Aes128Cbc_Key *key = &roundKey->round[10];
    uint8x16_t newState = addRoundKey(state, key);
    for (uint32_t round = 9; round > 0; --round) {
        --key;
        newState = invShiftRowsSubBytes(newState);
        newState = invMixColumns(newState);
        newState = addRoundKey(newState, key);
    }
    --key;
    newState = invShiftRowsSubBytes(newState);
    return addRoundKey(newState, key);
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
