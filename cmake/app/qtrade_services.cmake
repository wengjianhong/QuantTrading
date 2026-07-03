# ---------------------------------------------------------------------------
# Support microservices: aggregate includes and install target list
#
# Conventions:
#   - One .cmake per service: executable (+ optional *_static implementation lib)
#   - Executable target name = installed binary = config/<name>.json (without .json)
#   - Implementation lib: <service>_static, linkable from unit tests
# ---------------------------------------------------------------------------

# Control plane (implemented)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_config_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_account_service.cmake)

# Observability (MVP stub)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_log_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_monitor_service.cmake)

# Governance and others (MVP stub)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_registry_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_history_order_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_audit_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_backtest_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_backup_service.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/services/qtrade_strategy_service.cmake)

# For qtrade_install.cmake: install to bin/
set(QTRADE_SERVICE_TARGETS
    qtrade_config_service
    qtrade_account_service
    qtrade_log_service
    qtrade_monitor_service
    qtrade_registry_service
    qtrade_history_order_service
    qtrade_audit_service
    qtrade_backtest_service
    qtrade_backup_service
    qtrade_strategy_service
)
