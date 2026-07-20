/// @file      config_grpc_async_handler.cpp
/// @brief     ConfigService Async + CQ RPC 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/grpc/config_grpc_async_handler.hpp"

#include "qtrade/dao/dao_define.hpp"
#include "qtrade/framework/grpc/async/call_tag_base.hpp"
#include "qtrade/framework/grpc/async/unary_call_tag.hpp"

#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>

namespace qtrade::service {

namespace detail {

/// @brief SubscribeEngineConfig 轮询数据库的间隔（毫秒）
constexpr int kWatchPollIntervalMs = 2000;

using ConfigUnaryCallTag = qtrade::common::grpc_async::UnaryCallTag<qtrade::config::v1::ConfigService::AsyncService,
                                                                    ConfigGrpcAsyncHandler,
                                                                    qtrade::config::v1::GetEngineConfigRequest,
                                                                    qtrade::config::v1::GetEngineConfigResponse>;

/// @brief SubscribeEngineConfig 异步 CallTag（Server Streaming；定时查库推送）
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
      service_->RequestSubscribeEngineConfig(&ctx_, &request_, &writer_, cq_, cq_, this);
      return;
    }

    if (status_ == CallStatus::kAccept) {
      if (!accepted_) {
        accepted_ = true;
        scope_ = MakeConfigScope(request_);
        since_version_ = request_.since_version();
        handler_->SpawnSubscribeEngineConfig();
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
      since_version_ = outgoing_.engine().version();
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
    const auto config = handler_->QueryConfig(scope_);
    if (config.version() <= since_version_) {
      return;
    }
    *outgoing_.mutable_engine() = config;
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
  qtrade::config::v1::SubscribeEngineConfigRequest request_;
  qtrade::config::v1::SubscribeEngineConfigResponse outgoing_;
  grpc::ServerAsyncWriter<qtrade::config::v1::SubscribeEngineConfigResponse> writer_;
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

ConfigGrpcAsyncHandler::~ConfigGrpcAsyncHandler() {
  Shutdown();
}

void ConfigGrpcAsyncHandler::Init(qtrade::config::v1::ConfigService::AsyncService* async_service,
                                  grpc::ServerCompletionQueue* cq,
                                  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection,
                                  std::shared_ptr<qtrade::framework::dao::DaoManager> dao) {
  async_service_ = async_service;
  cq_ = cq;
  connection_pool_mgr_ = std::move(connection);
  dao_mgr_ = std::move(dao);
}

void ConfigGrpcAsyncHandler::Start() {
  if (started_ || async_service_ == nullptr || cq_ == nullptr || !DatabaseReady()) {
    return;
  }

  SpawnGetEngineConfig();
  SpawnSubscribeEngineConfig();

  started_ = true;
}

void ConfigGrpcAsyncHandler::Shutdown() {
  started_ = false;
}

void ConfigGrpcAsyncHandler::SpawnGetEngineConfig() {
  if (async_service_ == nullptr || cq_ == nullptr || !DatabaseReady()) {
    return;
  }
  new detail::ConfigUnaryCallTag(
    this,
    async_service_,
    cq_,
    &qtrade::config::v1::ConfigService::AsyncService::RequestGetEngineConfig,
    [](ConfigGrpcAsyncHandler* handler,
       const qtrade::config::v1::GetEngineConfigRequest& request,
       qtrade::config::v1::GetEngineConfigResponse* response) {
      const ConfigScope scope = MakeConfigScope(request);
      *response->mutable_engine() = handler->QueryConfig(scope);
      return grpc::Status::OK;
    },
    [](ConfigGrpcAsyncHandler* handler) { handler->SpawnGetEngineConfig(); });
}

void ConfigGrpcAsyncHandler::SpawnSubscribeEngineConfig() {
  if (async_service_ == nullptr || cq_ == nullptr || !DatabaseReady()) {
    return;
  }
  new detail::SubscribeConfigCallTag(this, async_service_, cq_);
}

qtrade::config::v1::EngineConfig ConfigGrpcAsyncHandler::QueryConfig(const ConfigScope& scope) const {
  if (dao_mgr_ == nullptr) {
    return qtrade::config::v1::EngineConfig{};
  }
  auto connection = connection_pool_mgr_->Acquire(qtrade::framework::dao::kConfigDatabaseName);
  if (connection == nullptr) {
    return qtrade::config::v1::EngineConfig{};
  }
  return QueryEngineConfig(scope, dao_mgr_->Get<qtrade::framework::dao::EngineConfig>(), *connection);
}

}  // namespace qtrade::service
