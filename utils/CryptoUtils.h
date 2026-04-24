#pragma once
#include <string>

// Hash de contraseñas usando PBKDF2-SHA256 con salt aleatorio de 16 bytes
// y 100.000 iteraciones. Formato almacenado: "<salt_hex>:<hash_hex>"
namespace CryptoUtils {
    std::string hashPassword(const std::string& plaintext);
    bool        verifyPassword(const std::string& plaintext, const std::string& stored);
}
