/*
    References:

    https://en.wikipedia.org/wiki/Advanced_Encryption_Standard
    https://en.wikipedia.org/wiki/AES_key_schedule
    https://en.wikipedia.org/wiki/Rijndael_S-box
    https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
*/

#include "Aes128Cbc.h"

struct State {
    uint8_t data[16];
};

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

static struct State
addRoundKey(const struct State *state, const struct Aes128Cbc_Key *key)
{
    struct State newState;

    uint32_t *out = (uint32_t *)newState.data;
    uint32_t *v = (uint32_t *)state->data;
    uint32_t *r = (uint32_t *)key->data;
    out[0] = v[0] ^ r[0];
    out[1] = v[1] ^ r[1];
    out[2] = v[2] ^ r[2];
    out[3] = v[3] ^ r[3];
    return newState;
}

static struct State
invShiftRowsSubBytes(const struct State *state)
{
    struct State newState;

    const uint8_t *s = (const uint8_t *)state->data;
    uint8_t *o = newState.data;
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F
    // |       |       |       |
    // 0 D A 7 4 1 E B 8 5 2 F C 9 6 3
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
    return newState;
}

static struct State
invMixColumns(const struct State *state)
{
    struct State newState;

    const uint32_t *data = (const uint32_t *)state->data;
    uint32_t *out = (uint32_t *)newState.data;

    for (int i = 0; i < 4; ++i) {
        uint32_t v = *data;
        uint8_t a0 = (uint8_t)v;
        uint8_t a1 = (uint8_t)(v >> 8);
        uint8_t a2 = (uint8_t)(v >> 16);
        uint8_t a3 = (uint8_t)(v >> 24);
        uint32_t b0 = MULTIPLY_0[a0];
        uint32_t b1 = MULTIPLY_1[a1];
        uint32_t b2 = MULTIPLY_2[a2];
        uint32_t b3 = MULTIPLY_3[a3];
        *out = b0 ^ b1 ^ b2 ^ b3;
        ++data;
        ++out;
    }
    return newState;
}

static struct State
xorWithIv(const struct State *state, const uint8_t *iv)
{
    struct State newState;

    uint32_t *o = (uint32_t *)newState.data;
    const uint32_t *p = (const uint32_t *)state->data;
    const uint32_t *q = (const uint32_t *)iv;
    o[0] = p[0] ^ q[0];
    o[1] = p[1] ^ q[1];
    o[2] = p[2] ^ q[2];
    o[3] = p[3] ^ q[3];
    return newState;
}

static void
postKeyExpansion(struct Aes128Cbc_RoundKey *roundKey)
{
    for (int k = 1; k < 10; ++k) {
        struct Aes128Cbc_Key *key = &roundKey->round[k];
        struct State state;
        uint32_t *o = (uint32_t *)key->data;
        uint32_t *s = (uint32_t *)state.data;
        s[0] = o[0];
        s[1] = o[1];
        s[2] = o[2];
        s[3] = o[3];
        struct State newState = invMixColumns(&state);
        uint32_t *n = (uint32_t *)newState.data;
        o[0] = n[0];
        o[1] = n[1];
        o[2] = n[2];
        o[3] = n[3];
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

static struct State
eqInvCipher(const struct State *state, const struct Aes128Cbc_RoundKey *roundKey)
{
    const struct Aes128Cbc_Key *key = &roundKey->round[10];
    struct State newState = addRoundKey(state, key);
    for (uint32_t round = 9; round > 0; --round) {
        --key;
        newState = invShiftRowsSubBytes(&newState);
        newState = invMixColumns(&newState);
        newState = addRoundKey(&newState, key);
    }
    --key;
    newState = invShiftRowsSubBytes(&newState);
    return addRoundKey(&newState, key);
}

void Aes128Cbc_decrypt(struct Aes128Cbc *ctx, const void *data,
    size_t length, void *output)
{
    const uint8_t *in = (const uint8_t *)data;
    const uint8_t *iv = ctx->iv.data;
    uint8_t *out = (uint8_t *)output;
    while (length > 0) {
        struct State state;
        {
            const uint32_t *v = (const uint32_t *)in;
            uint32_t *o = (uint32_t *)state.data;
            o[0] = v[0];
            o[1] = v[1];
            o[2] = v[2];
            o[3] = v[3];
        }
        state = eqInvCipher(&state, &ctx->roundKey);
        state = xorWithIv(&state, iv);
        {
            const uint32_t *v = (const uint32_t *)state.data;
            uint32_t *o = (uint32_t *)out;
            o[0] = v[0];
            o[1] = v[1];
            o[2] = v[2];
            o[3] = v[3];
        }
        iv = in;
        in += 16;
        out += 16;
        length -= 16;
    }
    uint32_t *o = (uint32_t *)ctx->iv.data;
    const uint32_t *p = (const uint32_t *)iv;
    o[0] = p[0];
    o[1] = p[1];
    o[2] = p[2];
    o[3] = p[3];
}
