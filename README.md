# MimicSSL-AES128-CBC-DECRYPT

This is an [AES][wikipedia::aes]-128 CBC implementation in C23 only for
decrypting, and its API is compatible with
[OpenSSL 1.1][openssl::EVP_DecryptInit_ex]. See [FIPS PUB 197][fips::197] for
the AES specifications.

Note that the current implementation works only on little-endian platforms.

## Example

An example usage would be as follows:

```c
int decrypt(FILE *in, FILE *out)
{
    unsigned char inbuf[1024], outbuf[1024];
    int inlen, outlen;
    EVP_CIPHER_CTX *ctx;
    unsigned char key[] = "0123456789abcdeF";
    unsigned char iv[] = "1234567887654321";

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        return 0;
    }
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    for (;;) {
        inlen = fread(inbuf, 1, 1024, in);
        if (inlen <= 0) {
            break;
        }
        if (!EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        fwrite(outbuf, 1, outlen, out);
    }
    if (!EVP_DecryptFinal_ex(ctx, outbuf, &outlen)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    if (outlen > 0) {
        fwrite(outbuf, 1, outlen, out);
    }
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}
```

## Build

This repository uses [lighter][maroontress::lighter] for testing as a sub-module
of Git. Therefore, clone it as follows:

```plaintext
git clone --recursive URL
```

Then build the library as follows:

```textplain
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

## Build for Android

Set environment variables `ANDROID_HOME` and `ANDROID_NDK` appropriately. For
example:

```sh
export ANDROID_HOME=/usr/local/lib/android/sdk
export ANDROID_NDK=$ANDROID_HOME/ndk/25.2.9519653
```

Note that the value of `ANDROID_HOME` will vary depending on your environment,
but a typical configuration would be as follows:

- Windows: `C:\\Users\\USERNAME\\AppData\\Local\\Android\\sdk`
- Linux: `/home/USERNAME/Android/Sdk`
- macOS: `/Users/USERNAME/Library/Android/sdk`

Then build as follows:

```
sh build-android.sh ABI -DBUILD_TESTSUITE=OFF
```

`ABI` should be replaced by `arm64-v8a`, `armeabi-v7a`, `x86`, or `x86_64`.

[wikipedia::aes]: https://en.wikipedia.org/wiki/Advanced_Encryption_Standard
[openssl::EVP_DecryptInit_ex]: https://www.openssl.org/docs/man1.1.1/man3/EVP_DecryptInit_ex.html
[fips::197]: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
[maroontress::lighter]: https://github.com/maroontress/lighter
