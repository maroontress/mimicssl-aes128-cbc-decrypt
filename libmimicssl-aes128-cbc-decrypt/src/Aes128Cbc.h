#ifndef Aes128Cbc_H
#define Aes128Cbc_H

#include <stddef.h>
#include <stdint.h>

struct Aes128Cbc_Iv {
    uint8_t data[16];
};

struct Aes128Cbc_Key {
    uint8_t data[16];
};

struct Aes128Cbc_RoundKey {
    struct Aes128Cbc_Key round[11];
};

struct Aes128Cbc {
    struct Aes128Cbc_RoundKey roundKey;
    struct Aes128Cbc_Iv iv;
};

#if defined(__cplusplus)
extern "C" {
#endif

void Aes128Cbc_init(struct Aes128Cbc *ctx, const struct Aes128Cbc_Key *key,
    const struct Aes128Cbc_Iv *iv);
void Aes128Cbc_decrypt(struct Aes128Cbc *ctx, const void *data,
    size_t length, void *output);
    
#if defined(__cplusplus)
}
#endif

#endif
