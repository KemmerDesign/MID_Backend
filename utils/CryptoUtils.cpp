#include "CryptoUtils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>

static constexpr int SALT_BYTES  = 16;
static constexpr int HASH_BYTES  = 32;   // SHA-256 output
static constexpr int ITERATIONS  = 100000;

static std::string toHex(const unsigned char* buf, size_t len) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
        ss << std::setw(2) << static_cast<unsigned>(buf[i]);
    return ss.str();
}

static std::vector<unsigned char> fromHex(const std::string& hex) {
    std::vector<unsigned char> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        unsigned int byte;
        std::istringstream(hex.substr(i * 2, 2)) >> std::hex >> byte;
        out[i] = static_cast<unsigned char>(byte);
    }
    return out;
}

std::string CryptoUtils::hashPassword(const std::string& plaintext) {
    unsigned char salt[SALT_BYTES];
    if (RAND_bytes(salt, SALT_BYTES) != 1)
        throw std::runtime_error("CryptoUtils: RAND_bytes falló");

    unsigned char hash[HASH_BYTES];
    if (PKCS5_PBKDF2_HMAC(
            plaintext.c_str(), static_cast<int>(plaintext.size()),
            salt, SALT_BYTES,
            ITERATIONS,
            EVP_sha256(),
            HASH_BYTES, hash) != 1)
        throw std::runtime_error("CryptoUtils: PBKDF2 falló");

    return toHex(salt, SALT_BYTES) + ":" + toHex(hash, HASH_BYTES);
}

bool CryptoUtils::verifyPassword(const std::string& plaintext, const std::string& stored) {
    size_t sep = stored.find(':');
    if (sep == std::string::npos) return false;

    auto salt = fromHex(stored.substr(0, sep));
    auto expectedHash = fromHex(stored.substr(sep + 1));

    unsigned char computedHash[HASH_BYTES];
    if (PKCS5_PBKDF2_HMAC(
            plaintext.c_str(), static_cast<int>(plaintext.size()),
            salt.data(), static_cast<int>(salt.size()),
            ITERATIONS,
            EVP_sha256(),
            HASH_BYTES, computedHash) != 1)
        return false;

    // Comparación en tiempo constante para evitar timing attacks
    return CRYPTO_memcmp(computedHash, expectedHash.data(), HASH_BYTES) == 0;
}
