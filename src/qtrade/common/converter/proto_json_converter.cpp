/// @file      proto_json_converter.cpp
/// @brief     protobuf Message 与 JSON 字符串互转实现
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/converter/proto_json_converter.hpp"

#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>

namespace qtrade::common::converter {

bool ConvertProtoToJson(const google::protobuf::Message& message,
                        std::string& out,
                        const ProtoJsonOptions& options,
                        const std::string_view log_tag) {
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

bool ConvertJsonToProto(const std::string& json,
                        google::protobuf::Message& message,
                        const ProtoJsonOptions& options,
                        const std::string_view log_tag) {
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

}  // namespace qtrade::common::converter
