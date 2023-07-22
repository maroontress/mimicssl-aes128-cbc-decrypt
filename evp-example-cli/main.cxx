#include <fstream>
#include <iostream>

#include <evp.h>

static unsigned char inbuf[1024 * 1024];
static unsigned char outbuf[1024 * 1024];

int
main(int ac, char** av)
{
    unsigned char key[16];
    unsigned char iv[16];
    int inlen;
    int outlen;
    EVP_CIPHER_CTX* ctx;

    if (ac != 5) {
        std::cerr << "usage: " << av[0]
                  << " KEY_FILE IV_FILE INPUT_FILE OUTPUT_FILE" << std::endl;
        return 1;
    }
    std::ifstream keyFile {av[1], std::ios_base::in | std::ios_base::binary};
    if (!keyFile) {
        std::cerr << av[1] << ": not found" << std::endl;
        return 1;
    }
    keyFile.read((char*)key, sizeof(key));
    if (keyFile.fail()) {
        std::cerr << av[1] << ": failed to read" << std::endl;
        return 1;
    }
    std::ifstream ivFile {av[2], std::ios_base::in | std::ios_base::binary};
    if (!ivFile) {
        std::cerr << av[2] << ": not found" << std::endl;
        return 1;
    }
    ivFile.read((char*)iv, sizeof(iv));
    if (ivFile.fail()) {
        std::cerr << av[2] << ": failed to read" << std::endl;
        return 1;
    }

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        std::cerr << "EVP_CIPHER_CTX_new(): failed" << std::endl;
        return 1;
    }
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        std::cerr << "EVP_DecryptInit_ex(): failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }

    std::ifstream inputFile {av[3], std::ios_base::in | std::ios_base::binary};
    if (!inputFile) {
        std::cout << av[3] << ": not found" << std::endl;
        return 1;
    }
    std::ofstream outputFile {av[4],
        std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
    if (!outputFile) {
        std::cout << av[4] << ": failed to open" << std::endl;
        return 1;
    }

    for (;;) {
        if (inputFile.eof()) {
            break;
        }
        inputFile.read((char*)inbuf, sizeof(inbuf));
        inlen = inputFile.gcount();
        if (!EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            std::cerr << "EVP_DecryptUpdate(): failed" << std::endl;
            EVP_CIPHER_CTX_free(ctx);
            return 1;
        }
        outputFile.write((char*)outbuf, outlen);
        if (outputFile.fail()) {
            std::cerr << av[4] << ": failed to write" << std::endl;
            return 1;
        }
    }
    if (!EVP_DecryptFinal_ex(ctx, outbuf, &outlen)) {
        std::cerr << "EVP_DecryptFinal_ex(): failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }
    if (outlen > 0) {
        outputFile.write((char*)outbuf, outlen);
        if (outputFile.fail()) {
            std::cout << av[4] << ": failed to write" << std::endl;
            return 1;
        }
    }
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}
