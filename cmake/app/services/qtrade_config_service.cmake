# ---------------------------------------------------------------------------
# Standalone process: config center (qtrade_config_service)
#
# Targets:
#   qtrade_config_service_static  implementation (src/service/config_service/)
#   qtrade_config_service         executable (src/apps/.../main.cpp)
#
# Config: config/qtrade_config_service.json
# Tests:  target_link_libraries(... PRIVATE qtrade_config_service_static)
# ---------------------------------------------------------------------------

# Recursively collect implementation sources (includes repository/)
file(GLOB_RECURSE QTRADE_SVC_CONFIG_SRC CONFIGURE_DEPENDS
    ${QTRADE_SRC_DIR}/service/config_service/*.cpp)

# Build Static Library
add_library(qtrade_config_service_static STATIC ${QTRADE_SVC_CONFIG_SRC})
target_include_directories(qtrade_config_service_static PUBLIC
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_config_service_static PUBLIC qtrade_common)

# Build Service Executable
add_executable(qtrade_config_service ${QTRADE_APPS_DIR}/qtrade_config_service/main.cpp)
target_link_libraries(qtrade_config_service PRIVATE qtrade_config_service_static)
