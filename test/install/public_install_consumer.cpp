#include <qtrade/common/boot/process_boot.hpp>
#include <qtrade/common/config/service_config.hpp>
#include <qtrade/engine/engine.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/quote/quote_spi.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>
#include <qtrade/sdk/trader/trader_spi.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/structs/result.hpp>

int main() {
  qtrade::Result<int> result;
  if (result.error_code != qtrade::ErrorCode::kSuccess) {
    return 1;
  }

  auto engine = qtrade::engine::CreateEngine();
  return engine ? 0 : 1;
}
