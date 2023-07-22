#ifndef evp_H
#define evp_H

#include "evp_export.h"

typedef struct EVP_CIPHER_CTX EVP_CIPHER_CTX;
typedef struct EVP_CIPHER EVP_CIPHER;
typedef struct ENGINE ENGINE;

#if defined(__cplusplus)
extern "C" {
#endif

EVP_CIPHER_CTX *EVP_EXPORT EVP_CIPHER_CTX_new(void);
int EVP_EXPORT EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *c);
void EVP_EXPORT EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *c);
int EVP_EXPORT EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx,
    const EVP_CIPHER *cipher, ENGINE *impl,
    const unsigned char *key,
    const unsigned char *iv);
int EVP_EXPORT EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx,
    unsigned char *out, int *outl, const unsigned char *in, int inl);
int EVP_EXPORT EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx,
    unsigned char *outm, int *outl);
const EVP_EXPORT EVP_CIPHER *EVP_aes_128_cbc(void);

#if defined(__cplusplus)
}
#endif

#endif
