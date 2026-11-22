#include <doctest/doctest.h>
#include "../../src/common/crypto/CCrypto.h"
#include <array>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr dword kSeed = 0x12345678u;
constexpr dword kRelaySeed = 0x7f000001u;
constexpr uint kLoginLength = 62;
constexpr uint kGameLength = 65;
constexpr uint kSelectLength = 3;
constexpr uint kFollowLength = 2;
constexpr uint kLoginStreamLength = kLoginLength + kSelectLength + kGameLength + kFollowLength;
constexpr uint kGameStreamLength = kGameLength + kFollowLength;
constexpr size_t kHexCharactersPerByte = 2;

struct RotationVector
{
    dword version;
    dword hi;
    dword lo;
    std::string_view loginStream;
    std::string_view gameStream;
};

// Ciphertexts captured by executing the original client send functions under qemu-i386.
// The 1.23 and original 1.25 executables share the first pair; 1.25.35g/h/i share 1.25.35.
constexpr RotationVector kVectors[] =
{
    {1230000u, 0xf1a372d5u, 0x3a1fd527u,
        "523cef13f5993860cea828ec10622cb1ff588be2568c6117acf1df4803a6f4a92c70d2047cda80a9a7b1a24485e5d54d01a7"
        "f4dd4903a6f4dd490326b48239ebd3dfff6794ec2360d49628e00c7873c59a2bffe2568c6117acf1df4803a6f4dd4903a674"
        "69cc80aab8a2b537f28aa72901a7f4dd4903a6f4dd490326b47d99ebd24ef34d",
        "43318067959f3e7bc18335e60d4958d48c2c8be2568c6117acf1df4803a6f4dd4903a60078da87aea9b0b533ea97b14d01a7"
        "f4dd4903a6f4dd490326b47d99ebd23daa"},
    {1253400u, 0x38ecedacu, 0x025720c6u,
        "52ddfe7a20121c93d645bf46a45950ee31de2952efb19e09c2a715cca0960db4c3e6f8f0673697c3f37aa6a7950c40e635dc"
        "2852ef31dea9124fe1b61d3782d72cc111f1be98f86c338440b547bcf065ab52a2ad104e61f63d58ea335f69f23f596a737f"
        "8ddf683f13964e2b1d9ccd7dca23d7ad10ce2156edb01e49e237dd28d2afe224",
        "43d0910e40141a88d96ea24cb972248b42aa2952efb19e09c2a715cca0960dc0a6958cf4633690c4fd7bb1d0fa7e24e635dc"
        "2852ef31dea9124fe1b61dc822d72d2344"},
    {1253500u, 0x383477bcu, 0x02345cc6u,
        "52ddfe7aa052bcc37e91d5f37eb4a6958c00c6a5144c60f63d58ea335f69728bdce9ff73a656271b1f0c9dba9b8b03c7a594"
        "0c40e635dca8124fe1b61d3782d72cc111f13ed8583c9b502a009d51061e168c4d5aebb39f09c2a715cca0968d00466574fc"
        "4cbfd8e7ffe07536131b8e5c5a6bf3bf19ca2357edb01e49e237dd28d2afe224",
        "43d0910ec054bad871bac8f9639fd2f0ff74c6a5144c60f63d58ea335f6972ffb99a8b77a256201c110d8acdf4f967c7a594"
        "0c40e635dca8124fe1b61dc822d72d2344"},
    {1253600u, 0x387fc5ccu, 0x021510c6u,
        "524e1e659b1430fb6e866065aa6000d91400d91400d91400d91400d91400d96065aa6070b86773ae7b72bd1400d91400d914"
        "00d91400d91400d91400d9eba0d91591a61400d8666fad7574b07b6e866065aa6000d91400d91400d91400d91400d91400d9"
        "6065aa6070b86773ae7b72bd1400d91400d91400d91400d91400d91400d9672a",
        "43437111fb1236e061ad7d6fb74b74bc6774d91400d91400d91400d91400d91400d91474bc6774a97573aa636fab7000d914"
        "00d91400d91400d91400d91400d91473f3"},
};

byte hexDigit(char c)
{
    constexpr byte kDecimalDigitCount = 10;
    return static_cast<byte>(c <= '9' ? c - '0' : c - 'a' + kDecimalDigitCount);
}

template<size_t N>
std::array<byte, N> decode(std::string_view hex)
{
    REQUIRE(hex.size() == N * kHexCharactersPerByte);
    std::array<byte, N> result{};
    for (size_t i = 0; i < N; ++i)
        result[i] = static_cast<byte>((hexDigit(hex[i * kHexCharactersPerByte]) << 4) | hexDigit(hex[i * kHexCharactersPerByte + 1]));
    return result;
}

// The length of a buffer, as the crypto functions take it.
template<size_t N>
constexpr uint len(const std::array<byte, N>&) noexcept
{
    return static_cast<uint>(N);
}

struct RotationKeys
{
    std::vector<CCryptoClientKey> saved;

    RotationKeys() : saved(std::move(CCryptoKeysHolder::get()->client_keys))
    {
        auto* holder = CCryptoKeysHolder::get();
        holder->addNoCryptKey();
        for (const auto& vector : kVectors)
            holder->client_keys.push_back({vector.version, vector.hi, vector.lo, ENC_LOGIN});
    }

    ~RotationKeys()
    {
        CCryptoKeysHolder::get()->client_keys = std::move(saved);
    }
};

constexpr std::string_view kLoginPlain = "80726f746174696f6e5f7465737400000000000000000000000000000000007465737470617373776f7264000000000000000000000000000000000000ff";
constexpr std::string_view kSelectPlain = "a00001";
constexpr std::string_view kGamePlain = "917f000001726f746174696f6e5f7465737400000000000000000000000000000000007465737470617373776f7264000000000000000000000000000000000000";
constexpr std::string_view kFollowPlain = "732a";
}

TEST_CASE("Early rotation cipher detects login and preserves the stream across relay")
{
    RotationKeys keys;
    const auto loginPlain = decode<kLoginLength>(kLoginPlain);
    const auto selectPlain = decode<kSelectLength>(kSelectPlain);
    const auto gamePlain = decode<kGameLength>(kGamePlain);
    const auto followPlain = decode<kFollowLength>(kFollowPlain);

    for (const auto& vector : kVectors)
    {
        INFO("Client version: " << vector.version);
        const auto encrypted = decode<kLoginStreamLength>(vector.loginStream);
        for (const bool split : {false, true})
        {
            INFO("Split input: " << split);
            CCrypto crypto;
            REQUIRE(crypto.Init(kSeed, encrypted.data(), kLoginLength));
            REQUIRE(crypto.IsInit());
            CHECK(crypto.GetConnectType() == CONNECT_LOGIN);
            REQUIRE(crypto.GetClientVerNumber() == vector.version);
            REQUIRE(crypto.GetEncryptionType() == ENC_LOGIN);

            std::array<byte, kLoginLength> login{};
            if (split)
            {
                constexpr uint kFirstChunk = 1;
                constexpr uint kSecondChunk = 5;
                REQUIRE(crypto.Decrypt(login.data(), encrypted.data(), kFirstChunk, kFirstChunk));
                REQUIRE(crypto.Decrypt(login.data() + kFirstChunk, encrypted.data() + kFirstChunk, kSecondChunk, kSecondChunk));
                constexpr uint kRestOffset = kFirstChunk + kSecondChunk;
                REQUIRE(crypto.Decrypt(login.data() + kRestOffset, encrypted.data() + kRestOffset, kLoginLength - kRestOffset, kLoginLength - kRestOffset));
            }
            else
                REQUIRE(crypto.Decrypt(login.data(), encrypted.data(), len(login), len(login)));
            CHECK(login == loginPlain);

            std::array<byte, kSelectLength> select{};
            REQUIRE(crypto.Decrypt(select.data(), encrypted.data() + kLoginLength, len(select), len(select)));
            CHECK(select == selectPlain);

            // A retained socket keeps its rotation masks despite the relay seed changing.
            crypto.InitFast(kRelaySeed, CONNECT_GAME);
            std::array<byte, kGameLength> game{};
            REQUIRE(crypto.Decrypt(game.data(), encrypted.data() + kLoginLength + kSelectLength, len(game), len(game)));
            CHECK(game == gamePlain);
            CHECK(crypto.GetConnectType() == CONNECT_GAME);
            std::array<byte, kFollowLength> follow{};
            REQUIRE(crypto.Decrypt(follow.data(), encrypted.data() + kLoginLength + kSelectLength + kGameLength, len(follow), len(follow)));
            CHECK(follow == followPlain);
        }
    }
}

TEST_CASE("Early rotation cipher detects a fresh game connection and accepts in-place chunks")
{
    RotationKeys keys;
    const auto gamePlain = decode<kGameLength>(kGamePlain);
    const auto followPlain = decode<kFollowLength>(kFollowPlain);
    for (const auto& vector : kVectors)
    {
        INFO("Client version: " << vector.version);
        auto encrypted = decode<kGameStreamLength>(vector.gameStream);
        CCrypto crypto;
        REQUIRE(crypto.Init(kSeed, encrypted.data(), kGameLength));
        REQUIRE(crypto.GetClientVerNumber() == vector.version);
        REQUIRE(crypto.GetEncryptionType() == ENC_LOGIN);
        CHECK(crypto.GetConnectType() == CONNECT_GAME);
        constexpr uint kFirstChunk = 17;
        REQUIRE(crypto.Decrypt(encrypted.data(), encrypted.data(), kFirstChunk, kFirstChunk));
        REQUIRE(crypto.Decrypt(encrypted.data() + kFirstChunk, encrypted.data() + kFirstChunk, kGameLength - kFirstChunk, kGameLength - kFirstChunk));
        CHECK(std::memcmp(encrypted.data(), gamePlain.data(), kGameLength) == 0);

        // Early clients receive plaintext (with Huffman compression handled by the network layer).
        // Sending server data must leave the independent client-to-server rotation masks untouched.
        const std::array<byte, kFollowLength> serverData{0x73, 0x42};
        std::array<byte, kFollowLength> serverOutput{};
        REQUIRE(crypto.Encrypt(serverOutput.data(), serverData.data(), len(serverOutput), len(serverData)));
        CHECK(serverOutput == serverData);
        REQUIRE(crypto.Decrypt(encrypted.data() + kGameLength, encrypted.data() + kGameLength, kFollowLength, kFollowLength));
        CHECK(std::memcmp(encrypted.data() + kGameLength, followPlain.data(), kFollowLength) == 0);
    }
}

TEST_CASE("Plaintext login accepts an output buffer of exactly the input size")
{
    RotationKeys keys;
    const auto login = decode<kLoginLength>(kLoginPlain);
    std::array<byte, kLoginLength> output{};
    CCrypto crypto;
    REQUIRE(crypto.Init(kSeed, login.data(), len(login)));
    REQUIRE(crypto.GetEncryptionType() == ENC_NONE);
    REQUIRE(crypto.Decrypt(output.data(), login.data(), len(output), len(login)));
    CHECK(output == login);
}

TEST_CASE("Plaintext game detects a fresh connection and accepts in-place follow-up packets")
{
    RotationKeys keys;
    const auto game = decode<kGameLength>(kGamePlain);
    auto output = game;
    CCrypto crypto;
    REQUIRE(crypto.Init(kSeed, game.data(), len(game)));
    REQUIRE(crypto.GetEncryptionType() == ENC_NONE);
    CHECK(crypto.GetConnectType() == CONNECT_GAME);
    REQUIRE(crypto.Decrypt(output.data(), output.data(), len(output), len(output)));
    CHECK(output == game);

    auto follow = decode<kFollowLength>(kFollowPlain);
    const auto originalFollow = follow;
    REQUIRE(crypto.Decrypt(follow.data(), follow.data(), len(follow), len(follow)));
    CHECK(follow == originalFollow);
}
