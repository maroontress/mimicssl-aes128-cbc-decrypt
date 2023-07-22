#if defined(__STDC_LIB_EXT1__) && (__STDC_LIB_EXT1__ >= 201112L)
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include <stdlib.h>
#include "libext1.h"

#include "evp.h"
#include "Aes128Cbc.h"

struct EVP_CIPHER_CTX {
    const EVP_CIPHER *cipher;
    void *data;
    uint8_t padding[16];
    uint32_t hasPadding;
};

struct EVP_CIPHER {
    void *(*newContext)(struct EVP_CIPHER_CTX *,
        const unsigned char *key, const unsigned char *iv);
    int (*update)(struct EVP_CIPHER_CTX *, void *,
        unsigned char *out, int *outl, const unsigned char *in, int inl);
    int (*finalize)(struct EVP_CIPHER_CTX *, unsigned char *outm, int *outl);
};

struct ENGINE {
    char pad;
};

EVP_CIPHER_CTX *
EVP_CIPHER_CTX_new(void)
{
    EVP_CIPHER_CTX *c = (EVP_CIPHER_CTX *)malloc(sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    c->cipher = NULL;
    c->data = NULL;
    c->hasPadding = 0;
    return c;
}

int
EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c)
{
    c->cipher = NULL;
    free(c->data);
    c->data = NULL;
    c->hasPadding = 0;
    return 1;
}

void
EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c)
{
    free(c->data);
    free(c);
}

static void *
aesNewContext(struct EVP_CIPHER_CTX *c,
    const unsigned char *key, const unsigned char *iv)
{
    struct Aes128Cbc *ctx = (struct Aes128Cbc *)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }
    struct Aes128Cbc_Key key0;
    struct Aes128Cbc_Iv iv0;
    MEMCPY(key0.data, key, 16);
    MEMCPY(iv0.data, iv, 16);
    Aes128Cbc_init(ctx, &key0, &iv0);
    c->hasPadding = 0;
    return ctx;
}

static int
aesUpdate(struct EVP_CIPHER_CTX *c,
    void *data, unsigned char *out, int *outl,
    const unsigned char *in, int inl)
{
    struct Aes128Cbc *ctx = (struct Aes128Cbc *)data;
    if (inl < 0 || (inl % 16) != 0) {
        return 0;
    }
    if (inl == 0) {
        return 1;
    }
    int outSize = 0;
    if (c->hasPadding) {
        MEMCPY(out, c->padding, 16);
        out += 16;
        outSize += 16;
    }
    int mainSize = inl - 16;
    if (mainSize > 0) {
        Aes128Cbc_decrypt(ctx, in, mainSize, out);
        outSize += mainSize;
        in += mainSize;
    }
    Aes128Cbc_decrypt(ctx, in, 16, c->padding);
    c->hasPadding = 1;
    *outl = outSize;
    return 1;
}

static int
aesFinalize(struct EVP_CIPHER_CTX *c,
    unsigned char *outm, int *outl)
{
    if (!c->hasPadding) {
        return 0;
    }
    const uint8_t *p = c->padding;
    for (uint32_t start = 0; start < 0x10; ++start) {
        uint8_t padValue = (uint8_t)(0x10 - start);
        uint32_t k = start;
        while (k < 0x10 && p[k] == padValue) {
            ++k;
        }
        if (k == 0x10) {
            *outl = start;
            if (start > 0) {
                MEMCPY(outm, c->padding, start);
            }
            return 1;
        }
    }
    return 0;
}

static const EVP_CIPHER aes128cbc = {
    .newContext = aesNewContext,
    .update = aesUpdate,
    .finalize = aesFinalize};

const EVP_CIPHER *
EVP_aes_128_cbc(void)
{
    return &aes128cbc;
}

int
EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
    ENGINE *impl, const unsigned char *key, const unsigned char *iv)
{
    if (ctx->cipher != NULL || impl != NULL) {
        return 0;
    }
    ctx->cipher = cipher;
    void *data = cipher->newContext(ctx, key, iv);
    if (data == NULL) {
        return 0;
    }
    ctx->data = data;
    return 1;
}
int
EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
    const unsigned char *in, int inl)
{
    return ctx->cipher->update(ctx, ctx->data, out, outl, in, inl);
}

int
EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl)
{
    return ctx->cipher->finalize(ctx, outm, outl);
}
