/// @file      engine_config_codec.cpp
/// @brief     EngineConfig JSON 编解码实现
/// @author    wengjianhong
/// @date      2026-07-02
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/repository/engine_config_codec.hpp"

#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace qtrade::service {

bool EngineConfigToJson(const qtrade::config::v1::EngineConfig& config, std::string& out) {
  google::protobuf::util::JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;
  const auto status = google::protobuf::util::MessageToJsonString(config, &out, options);
  if (!status.ok()) {
    spdlog::warn("[EngineConfigCodec] serialize failed: {}", status.ToString());
    return false;
  }
  return true;
}

bool EngineConfigFromJson(const std::string& json, qtrade::config::v1::EngineConfig& config) {
  config.Clear();
  google::protobuf::util::JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  const auto status = google::protobuf::util::JsonStringToMessage(json, &config, options);
  if (!status.ok()) {
    spdlog::warn("[EngineConfigCodec] parse failed: {}", status.ToString());
    return false;
  }
  return true;
}

}  // namespace qtrade::service
