/*
    References:

    https://en.wikipedia.org/wiki/Advanced_Encryption_Standard
    https://en.wikipedia.org/wiki/AES_key_schedule
    https://en.wikipedia.org/wiki/Rijndael_S-box
    https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
*/

/*
    References of SSE2, SSE3, and AES-NI Intrinsics:
    https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
*/

#include <emmintrin.h>
#include <pmmintrin.h>
#include <wmmintrin.h>
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
        __m128i *data = (__m128i *)key->data;
        __m128i key128 = _mm_aesimc_si128(_mm_lddqu_si128(data));
        _mm_storeu_si128(data, key128);
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

static __m128i
eqInvCipher(__m128i state, const struct Aes128Cbc_RoundKey *roundKey)
{
    const struct Aes128Cbc_Key *round = roundKey->round;

    state = _mm_xor_si128(state, _mm_lddqu_si128((const __m128i *)round[10].data));
    for (uint32_t k = 9; k > 0; --k) {
        state = _mm_aesdec_si128(state, _mm_lddqu_si128((const __m128i *)round[k].data));
    }
    return _mm_aesdeclast_si128(state, _mm_lddqu_si128((const __m128i *)round[0].data));
}

void Aes128Cbc_decrypt(struct Aes128Cbc *ctx, const void *data,
    size_t length, void *output)
{
    const uint8_t *in = (const uint8_t *)data;
    uint8_t *out = (uint8_t *)output;
    __m128i iv128 = _mm_lddqu_si128((const __m128i *)ctx->iv.data);
    while (length > 0) {
        __m128i in128 = _mm_lddqu_si128((const __m128i *)in);
        __m128i state = eqInvCipher(in128, &ctx->roundKey);
        _mm_storeu_si128((__m128i *)out, _mm_xor_si128(state, iv128));
        iv128 = in128;
        in += 16;
        out += 16;
        length -= 16;
    }
    _mm_storeu_si128((__m128i *)ctx->iv.data, iv128);
}
