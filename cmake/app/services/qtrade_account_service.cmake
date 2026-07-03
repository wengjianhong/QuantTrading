# ---------------------------------------------------------------------------
# Standalone process: trading account service (qtrade_account_service)
#
# Targets:
#   qtrade_account_service_static  implementation (src/service/account_service/)
#   qtrade_account_service           executable (src/apps/.../main.cpp)
#
# Config: config/qtrade_account_service.json
# Tests:  target_link_libraries(... PRIVATE qtrade_account_service_static)
# ---------------------------------------------------------------------------

# Recursively collect implementation sources (includes repository/)
file(GLOB_RECURSE QTRADE_SVC_ACCOUNT_SRC CONFIGURE_DEPENDS
    ${QTRADE_SRC_DIR}/service/account_service/*.cpp)

# Build Static Library
add_library(qtrade_account_service_static STATIC ${QTRADE_SVC_ACCOUNT_SRC})
target_include_directories(qtrade_account_service_static PUBLIC
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_account_service_static PUBLIC qtrade_common)

# Build Service Executable
add_executable(qtrade_account_service ${QTRADE_APPS_DIR}/qtrade_account_service/main.cpp)
target_link_libraries(qtrade_account_service PRIVATE qtrade_account_service_static)
