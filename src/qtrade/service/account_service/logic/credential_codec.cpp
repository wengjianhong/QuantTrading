/// @file      credential_codec.cpp
/// @brief     凭证加解密占位实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/logic/credential_codec.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::service {

bool EncryptCredential(const std::string& plaintext, std::string& key_id, std::string& ciphertext) {
  if (plaintext.empty()) {
    spdlog::warn("[CredentialCodec] empty plaintext");
    return false;
  }
  key_id = "stub-v1";
  ciphertext = plaintext;
  return true;
}

bool DecryptCredential(const std::string& key_id, const std::string& ciphertext, std::string& plaintext) {
  if (key_id.empty() || ciphertext.empty()) {
    spdlog::warn("[CredentialCodec] empty key_id or ciphertext");
    return false;
  }
  (void)key_id;
  plaintext = ciphertext;
  return true;
}

}  // namespace qtrade::service
