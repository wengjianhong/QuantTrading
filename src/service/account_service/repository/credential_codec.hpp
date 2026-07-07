/// @file      credential_codec.hpp
/// @brief     凭证加解密（框架占位，二期接 KMS）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CREDENTIAL_CODEC_HPP_
#define QTRADE_SERVICE_CREDENTIAL_CODEC_HPP_

#include <string>

namespace qtrade::service {

/// @brief 加密明文密码（当前为占位实现）
[[nodiscard]] bool EncryptCredential(const std::string& plaintext, std::string& key_id, std::string& ciphertext);

/// @brief 解密密文密码
[[nodiscard]] bool DecryptCredential(const std::string& key_id, const std::string& ciphertext, std::string& plaintext);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CREDENTIAL_CODEC_HPP_
