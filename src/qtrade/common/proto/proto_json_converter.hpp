/// @file      proto_json_converter.hpp
/// @brief     protobuf Message 与 JSON 字符串互转（通用模板）
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_PROTO_PROTO_JSON_CONVERTER_HPP_
#define QTRADE_COMMON_PROTO_PROTO_JSON_CONVERTER_HPP_

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>
#include <type_traits>

namespace qtrade::common {

/// @brief Proto JSON 编解码选项
struct ProtoJsonOptions {
  /// 序列化时输出零值 primitive 字段
  bool always_print_primitive_fields = true;
  /// 序列化时保留 proto 字段名
  bool preserve_proto_field_names = true;
  /// 反序列化时枚举大小写不敏感
  bool case_insensitive_enum_parsing = true;
};

/// @brief 将 protobuf Message 转换为 JSON 字符串
template <typename MessageT>
  requires std::is_base_of_v<google::protobuf::Message, MessageT>
[[nodiscard]] bool ConvertProtoToJson(const MessageT& message,
                                      std::string& out,
                                      const ProtoJsonOptions& options = {},
                                      const std::string_view log_tag = "ConvertProtoToJson") {
  google::protobuf::util::JsonPrintOptions print_options;
  print_options.always_print_primitive_fields = options.always_print_primitive_fields;
  print_options.preserve_proto_field_names = options.preserve_proto_field_names;
  const auto status = google::protobuf::util::MessageToJsonString(message, &out, print_options);
  if (!status.ok()) {
    spdlog::warn("[{}] convert proto to json failed: {}", log_tag, status.ToString());
    return false;
  }
  return true;
}

/// @brief 从 JSON 字符串转换为 protobuf Message
template <typename MessageT>
  requires std::is_base_of_v<google::protobuf::Message, MessageT>
[[nodiscard]] bool ConvertJsonToProto(const std::string& json,
                                      MessageT& message,
                                      const ProtoJsonOptions& options = {},
                                      const std::string_view log_tag = "ConvertJsonToProto") {
  message.Clear();
  google::protobuf::util::JsonParseOptions parse_options;
  parse_options.case_insensitive_enum_parsing = options.case_insensitive_enum_parsing;
  const auto status = google::protobuf::util::JsonStringToMessage(json, &message, parse_options);
  if (!status.ok()) {
    spdlog::warn("[{}] convert json to proto failed: {}", log_tag, status.ToString());
    return false;
  }
  return true;
}

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_PROTO_PROTO_JSON_CONVERTER_HPP_
