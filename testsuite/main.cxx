#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>

#include "expect.hxx"

class Driver final {
private:
    std::map<std::string, std::function<void()>> map;
    bool list = false;
    std::optional<std::string> name;

public:
    Driver(const char* const* args) {
        for (auto* a = args; *a != nullptr; ++a) {
            if (std::strcmp(*a, "--gtest_list_tests") == 0) {
                list = true;
                return;
            }
        }
        for (auto* a = args; *a != nullptr; ++a) {
            auto o = std::string {*a};
            auto prefix = std::string {"--gtest_filter=main."};
            if (o.starts_with(prefix)) {
                name = std::make_optional(o.substr(prefix.length()));
                return;
            }
        }
    }

    void add(const std::string& name, const std::function<void()>& testcase) {
        map[name] = testcase;
    }

    int run() const {
        if (list) {
            std::cout << "main.\n";
            for (auto i = map.cbegin(); i != map.cend(); ++i) {
                auto [name, testcase] = *i;
                std::cout << "  " << name << "\n";
            }
            return 0;
        }
        if (name.has_value()) {
            std::string n = name.value();
            auto testcase = map.at(n);
            try {
                testcase();
                std::cout << n << ": succeeded\n";
            } catch (std::runtime_error& e) {
                std::cout << n << ": failed\n";
                std::cout << "  " << e.what();
                return 1;
            }
            return 0;
        }
        auto count = 0;
        for (auto i = map.cbegin(); i != map.cend(); ++i) {
            auto [name, testcase] = *i;
            try {
                testcase();
                std::cout << name << ": succeeded\n";
            } catch (std::runtime_error& e) {
                std::cout << name << ": failed\n";
                std::cout << "  " << e.what();
                ++count;
            }
        }
        if (count == 0) {
            std::cout << "all tests passed\n";
            return 0;
        }
        std::cout << count << " test(s) failed.\n";
        return 1;
    }
};

namespace maroontress::lighter {
    template <> std::string toString(std::uint8_t b) {
        auto v = (uint32_t)b;
        std::ostringstream out;
        out << std::dec << v << " (0x" << std::hex << v << ")";
        return out.str();
    }
}

static auto
toArray(const std::string& m) -> std::array<std::uint8_t, 16>
{
    std::uint8_t array[16];

    if (m.length() != 32) {
        throw std::runtime_error("invalid length");
    }
    for (auto k = 0; k < 16; ++k) {
        auto p = m.substr(k * 2, 2);
        array[k] = (std::uint8_t)std::stoull(p, nullptr, 16);
    }
    return std::to_array(array);
}

#include "Aes128Cbc.h"
#include "Aes128Cbc.c"
#include "evp.h"

static auto
toState(const std::string& m) -> State
{
    struct State state;

    auto array = toArray(m);
    uint32_t *v = (uint32_t *)array.data();
    uint32_t *o = (uint32_t *)state.data;
    o[0] = v[0];
    o[1] = v[1];
    o[2] = v[2];
    o[3] = v[3];
    return state;
}

static const bool dumpEnabled = false;

static void
dump(const uint8_t* b)
{
    if constexpr (!dumpEnabled) {
        return;
    }
    for (int k = 0; k < 16; ++k) {
        std::printf("%02x", *b);
        ++b;
    }
    std::printf("\n");
}

static void
dump(const State& state)
{
    dump(state.data);
}

static void
checkKeyExpansion(std::array<std::uint8_t, 16>& key,
    std::array<const char*, 11>& expectedList)
{
    struct Aes128Cbc_Key k;
    std::memcpy(k.data, &key[0], 16);
    struct Aes128Cbc_RoundKey out;
    keyExpansion(&k, &out);
    for (int i = 0; i <= 10; ++i) {
        dump(out.round[i].data);
    }
    for (auto j = 0; j <= 10; ++j) {
        auto expected = toArray(expectedList[j]);
        auto actual = out.round[j].data;
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    }
}

template <std::size_t N>
static int
decrypt(FILE* in, FILE* out)
{
    static_assert(N % 16 == 0);
    unsigned char inbuf[N];
    unsigned char outbuf[N];
    int inlen;
    int outlen;
    EVP_CIPHER_CTX* ctx;
    // KEY "0123456789abcdeF" -> 30313233343536373839616263646546
    unsigned char key[] = "0123456789abcdeF";
    // IV "1234567887654321" -> 31323334353637383837363534333231
    unsigned char iv[] = "1234567887654321";

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
        return 0;
    }
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    for (;;) {
        inlen = fread(inbuf, 1, sizeof(inbuf), in);
        if (inlen <= 0) {
            break;
        }
        if (!EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            EVP_CIPHER_CTX_free(ctx);
            return 0;
        }
        if (outlen > 0) {
            fwrite(outbuf, 1, outlen, out);
        }
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

static void
compare(const char* fileOne, const char* fileTwo)
{
    auto sizeOfOne = std::filesystem::file_size(fileOne);
    auto sizeOfTwo = std::filesystem::file_size(fileTwo);
    expect(sizeOfOne) == sizeOfTwo;
    std::ifstream one {fileOne, std::ios_base::in | std::ios_base::binary};
    std::ifstream two {fileTwo, std::ios_base::in | std::ios_base::binary};
    expect(!one).isFalse();
    expect(!two).isFalse();
    for (;;) {
        if (one.eof()) {
            break;
        }
        auto charOne = one.get();
        auto charTwo = two.get();
        expect(charOne) == charTwo;
    }
}

int
main(int ac, char** av)
{
    auto driver = Driver {av};
    driver.add("endian", [] {
        expect(std::endian::native) == std::endian::little;
    });
    driver.add("keyExpansion (test vector)", [] {
        auto key = toArray("2b7e151628aed2a6abf7158809cf4f3c");
        std::array<const char*, 11> expectedList = {
            "2b7e151628aed2a6abf7158809cf4f3c",
            "a0fafe1788542cb123a339392a6c7605",
            "f2c295f27a96b9435935807a7359f67f",
            "3d80477d4716fe3e1e237e446d7a883b",
            "ef44a541a8525b7fb671253bdb0bad00",
            "d4d1c6f87c839d87caf2b8bc11f915bc",
            "6d88a37a110b3efddbf98641ca0093fd",
            "4e54f70e5f5fc9f384a64fb24ea6dc4f",
            "ead27321b58dbad2312bf5607f8d292f",
            "ac7766f319fadc2128d12941575c006e",
            "d014f9a8c9ee2589e13f0cc8b6630ca6"};
        checkKeyExpansion(key, expectedList);
    });
    driver.add("keyExpansion", [] {
        auto key = toArray("d41d8cd98f00b204e9800998ecf8427e");
        std::array<const char*, 11> expectedList = {
            "d41d8cd98f00b204e9800998ecf8427e",
            "94317f171b31cd13f2b1c48b1e4986f5",
            "ad759965b644547644f590fd5abc1608",
            "cc32a9db7a76fdad3e836d50643f7b58",
            "b113c398cb653e35f5e6536591d9283d",
            "9427e4195f42da2caaa489493b7da174",
            "4b1576fb1457acd7bef3259e858e84ea",
            "124af16c061d5dbbb8ee78253d60fccf",
            "42fa7b4b44e726f0fc095ed5c169a21a",
            "a0c0d933e427ffc3182ea116d947030c",
            "36bb2706d29cd8c5cab279d313f57adf"};
        checkKeyExpansion(key, expectedList);
    });
    driver.add("addRoundKey", [] {
        auto state = toState("000102030405060708090a0b0c0d0e0f");
        auto key = toArray("d41d8cd98f00b204e9800998ecf8427e");
        auto newState = addRoundKey(&state, key.data());
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("d41c8eda8b05b403e1890393e0f54c71");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("invShiftRows", [] {
        auto state = toState("d41d8cd98f00b204e9800998ecf8427e");
        auto newState = invShiftRows(&state);
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("d4f809048f1d4298e9008c7eec80b2d9");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("invSubBytes", [] {
        auto state = toState("d41d8cd98f00b204e9800998ecf8427e");
        auto newState = invSubBytes(&state);
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("19def0e573523e30eb3a40e283e1f68a");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("invMixColumns", [] {
        auto state = toState("d41d8cd98f00b204e9800998ecf8427e");
        auto newState = invMixColumns(&state);
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("36094dee9485dbf3ef90e46339cac71c");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("xorWithIv", [] {
        auto state = toState("000102030405060708090a0b0c0d0e0f");
        auto iv = toArray("d41d8cd98f00b204e9800998ecf8427e");
        auto newState = xorWithIv(&state, iv.data());
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("d41c8eda8b05b403e1890393e0f54c71");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("invCipher (test vector)", [] {
        auto state = toState("69c4e0d86a7b0430d8cdb78070b4c55a");
        auto key = toArray("000102030405060708090a0b0c0d0e0f");
        struct Aes128Cbc_Key k;
        std::memcpy(k.data, &key[0], 16);
        struct Aes128Cbc_RoundKey roundKey;
        keyExpansion(&k, &roundKey);
        auto newState = invCipher(&state, &roundKey);
        dump(newState);
        auto actual = newState.data;
        auto expected = toArray("00112233445566778899aabbccddeeff");
        for (auto k = 0; k < 16; ++k) {
            expect(actual[k]) == expected[k];
        }
    });
    driver.add("alice (1024 bytes at a time)", [] {
        auto* in = std::fopen("alice.md.encrypted", "rb");
        expect(in) != nullptr;
        auto* out = std::fopen("alice.md", "wb");
        expect(out) != nullptr;
        auto result = decrypt<1024>(in, out);
        expect(result) == 1;
        std::fclose(in);
        std::fclose(out);
        compare("alice.md", "alice.md.decrypted");
    });
    driver.add("alice (16 bytes at a time)", [] {
        auto* in = std::fopen("alice.md.encrypted", "rb");
        expect(in) != nullptr;
        auto* out = std::fopen("alice.md", "wb");
        expect(out) != nullptr;
        auto result = decrypt<16>(in, out);
        expect(result) == 1;
        std::fclose(in);
        std::fclose(out);
        compare("alice.md", "alice.md.decrypted");
    });
    return driver.run();
}
