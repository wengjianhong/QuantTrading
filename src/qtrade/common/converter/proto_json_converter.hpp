/// @file      proto_json_converter.hpp
/// @brief     protobuf Message 与 JSON 字符串互转
/// @details   封装 google::protobuf::util 的 JSON 编解码，供 config-service 持久化载荷、
///            调试输出等场景使用。失败时写 warn 日志并返回 false，不抛异常。
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONVERTER_PROTO_JSON_CONVERTER_HPP_
#define QTRADE_COMMON_CONVERTER_PROTO_JSON_CONVERTER_HPP_

#include <google/protobuf/message.h>

#include <string>
#include <string_view>

namespace qtrade::common::converter {

/// @brief Proto JSON 编解码选项
struct ProtoJsonOptions {
  /// 序列化时输出零值 primitive 字段（便于 diff 与人工排查）
  bool always_print_primitive_fields = true;
  /// 序列化时保留 proto 原始字段名（不使用 lowerCamelCase）
  bool preserve_proto_field_names = true;
  /// 反序列化时枚举字面量大小写不敏感
  bool case_insensitive_enum_parsing = true;
};

/// @brief 将 protobuf Message 序列化为 JSON 字符串
/// @param message 待序列化的 proto 消息（任意 Message 子类）
/// @param out 成功时写入 JSON 文本；失败时不保证 out 内容
/// @param options 编解码选项；默认适合配置快照落库/展示
/// @param log_tag 失败日志前缀，便于定位调用方
/// @return 序列化成功返回 true
[[nodiscard]] bool ConvertProtoToJson(const google::protobuf::Message& message,
                                      std::string& out,
                                      const ProtoJsonOptions& options = {},
                                      std::string_view log_tag = "ConvertProtoToJson");

/// @brief 从 JSON 字符串反序列化到 protobuf Message
/// @param json 输入 JSON 文本
/// @param message 目标 proto 消息；调用前会先 Clear，再按 json 填充字段
/// @param options 编解码选项
/// @param log_tag 失败日志前缀，便于定位调用方
/// @return 反序列化成功返回 true；json 非法或字段类型不匹配时返回 false
[[nodiscard]] bool ConvertJsonToProto(const std::string& json,
                                      google::protobuf::Message& message,
                                      const ProtoJsonOptions& options = {},
                                      std::string_view log_tag = "ConvertJsonToProto");

}  // namespace qtrade::common::converter

#endif  // QTRADE_COMMON_CONVERTER_PROTO_JSON_CONVERTER_HPP_
