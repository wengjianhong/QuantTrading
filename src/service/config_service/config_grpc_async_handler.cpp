/// @file      config_grpc_async_handler.cpp
/// @brief     ConfigService Async + CQ RPC 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "service/config_service/config_grpc_async_handler.hpp"

#include "common/grpc/call_tag_base.hpp"
#include "common/grpc/unary_call_tag.hpp"

#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>

namespace qtrade::service {

namespace detail {

/// @brief SubscribeConfig 轮询数据库的间隔（毫秒）
constexpr int kWatchPollIntervalMs = 2000;

using ConfigUnaryCallTag = qtrade::common::grpc_async::UnaryCallTag<qtrade::config::v1::ConfigService::AsyncService,
                                                                    ConfigGrpcAsyncHandler,
                                                                    qtrade::config::v1::GetConfigRequest,
                                                                    qtrade::config::v1::ConfigSnapshot>;

/// @brief SubscribeConfig 异步 CallTag（Server Streaming；定时查库推送）
class SubscribeConfigCallTag final : public qtrade::common::grpc_async::CallTagBase {
 public:
  SubscribeConfigCallTag(ConfigGrpcAsyncHandler* handler,
                         qtrade::config::v1::ConfigService::AsyncService* service,
                         grpc::ServerCompletionQueue* cq)
    : handler_(handler), service_(service), cq_(cq), writer_(&ctx_) {
    Proceed(true);
  }

  void Proceed(bool ok) override {
    if (!ok && status_ != CallStatus::kCreate) {
      Finish(grpc::Status(grpc::StatusCode::CANCELLED, "cancelled"));
      return;
    }

    if (status_ == CallStatus::kCreate) {
      status_ = CallStatus::kAccept;
      service_->RequestSubscribeConfig(&ctx_, &request_, &writer_, cq_, cq_, this);
      return;
    }

    if (status_ == CallStatus::kAccept) {
      if (!accepted_) {
        accepted_ = true;
        scope_ = MakeConfigScope(request_);
        since_version_ = request_.since_version();
        handler_->SpawnSubscribeConfig();
      }
      PollAndMaybeWrite();
      if (!finished_ && !write_in_flight_) {
        SchedulePoll();
      }
      return;
    }

    if (status_ == CallStatus::kWrite) {
      write_in_flight_ = false;
      if (!ok) {
        Finish(grpc::Status(grpc::StatusCode::CANCELLED, "write failed"));
        return;
      }
      since_version_ = outgoing_.version();
      status_ = CallStatus::kAccept;
      if (!finished_) {
        SchedulePoll();
      }
      return;
    }

    if (status_ == CallStatus::kFinish) {
      delete this;
    }
  }

 private:
  enum class CallStatus { kCreate, kAccept, kWrite, kFinish };

  /// @brief 调度下一次数据库轮询
  void SchedulePoll() {
    if (finished_) {
      return;
    }
    const gpr_timespec deadline =
      gpr_time_add(gpr_now(GPR_CLOCK_MONOTONIC), gpr_time_from_millis(kWatchPollIntervalMs, GPR_TIMESPAN));
    alarm_.Set(cq_, deadline, this);
  }

  /// @brief 查库并在版本更新时推送快照
  void PollAndMaybeWrite() {
    if (finished_ || write_in_flight_) {
      return;
    }
    const auto snapshot = handler_->QuerySnapshot(scope_);
    if (snapshot.version() <= since_version_) {
      return;
    }
    outgoing_ = snapshot;
    write_in_flight_ = true;
    status_ = CallStatus::kWrite;
    writer_.Write(outgoing_, this);
  }

  void Finish(const grpc::Status& status) {
    if (finished_) {
      return;
    }
    finished_ = true;
    alarm_.Cancel();
    status_ = CallStatus::kFinish;
    writer_.Finish(status, this);
  }

  ConfigGrpcAsyncHandler* handler_;
  qtrade::config::v1::ConfigService::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;
  qtrade::config::v1::SubscribeConfigRequest request_;
  qtrade::config::v1::ConfigSnapshot outgoing_;
  grpc::ServerAsyncWriter<qtrade::config::v1::ConfigSnapshot> writer_;
  grpc::Alarm alarm_;
  ConfigScope scope_;
  CallStatus status_ = CallStatus::kCreate;
  std::uint64_t since_version_ = 0;
  bool accepted_ = false;
  bool write_in_flight_ = false;
  bool finished_ = false;
};

}  // namespace detail

ConfigGrpcAsyncHandler::ConfigGrpcAsyncHandler() = default;

ConfigGrpcAsyncHandler::~ConfigGrpcAsyncHandler() { Shutdown(); }

void ConfigGrpcAsyncHandler::Init(qtrade::config::v1::ConfigService::AsyncService* async_service,
                                  grpc::ServerCompletionQueue* cq,
                                  std::shared_ptr<IConfigRepository> repository) {
  async_service_ = async_service;
  cq_ = cq;
  repository_ = std::move(repository);
}

void ConfigGrpcAsyncHandler::Start() {
  if (started_ || async_service_ == nullptr || cq_ == nullptr || !repository_) {
    return;
  }

  SpawnGetConfig();
  SpawnSubscribeConfig();

  started_ = true;
}

void ConfigGrpcAsyncHandler::Shutdown() { started_ = false; }

void ConfigGrpcAsyncHandler::SpawnGetConfig() {
  if (async_service_ == nullptr || cq_ == nullptr || !repository_) {
    return;
  }
  new detail::ConfigUnaryCallTag(
    this,
    async_service_,
    cq_,
    &qtrade::config::v1::ConfigService::AsyncService::RequestGetConfig,
    [](ConfigGrpcAsyncHandler* handler,
       const qtrade::config::v1::GetConfigRequest& request,
       qtrade::config::v1::ConfigSnapshot* response) {
      const ConfigScope scope = MakeConfigScope(request);
      *response = handler->QuerySnapshot(scope);
      return grpc::Status::OK;
    },
    [](ConfigGrpcAsyncHandler* handler) { handler->SpawnGetConfig(); });
}

void ConfigGrpcAsyncHandler::SpawnSubscribeConfig() {
  if (async_service_ == nullptr || cq_ == nullptr || !repository_) {
    return;
  }
  new detail::SubscribeConfigCallTag(this, async_service_, cq_);
}

qtrade::config::v1::ConfigSnapshot ConfigGrpcAsyncHandler::QuerySnapshot(const ConfigScope& scope) const {
  return QueryConfigSnapshot(repository_.get(), scope);
}

}  // namespace qtrade::service
