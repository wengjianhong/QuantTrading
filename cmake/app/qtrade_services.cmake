# ---------------------------------------------------------------------------
# Support microservices: static lib + executable per service
#
# Conventions:
#   - qtrade_add_support_service(<executable>, <src/qtrade/service impl dir>)
#   - Executable / install binary / config/<name>.json share the same name
#   - Implementation lib: <name>_static (linkable from unit tests)
# ---------------------------------------------------------------------------

# @brief 添加支撑微服务（static lib + executable）
# @param executable_name 可执行目标名，如 qtrade_log_service
# @param service_impl_dir 实现目录名，对应 src/qtrade/service/<dir>/
function(qtrade_add_support_service executable_name service_impl_dir)
  set(static_target "${executable_name}_static")

  file(GLOB_RECURSE _svc_src CONFIGURE_DEPENDS
    "${QTRADE_SRC_QTRADE_DIR}/service/${service_impl_dir}/*.cpp")
  if(NOT _svc_src)
    message(FATAL_ERROR "No .cpp under src/qtrade/service/${service_impl_dir}/")
  endif()

  add_library(${static_target} STATIC ${_svc_src})
  target_include_directories(${static_target}
    PUBLIC ${QTRADE_SRC_DIR}
    PRIVATE ${QTRADE_INCLUDE_DIR}
  )
  target_link_libraries(${static_target} PUBLIC qtrade_common)

  add_executable(${executable_name} "${QTRADE_APPS_DIR}/${executable_name}/main.cpp")
  target_link_libraries(${executable_name} PRIVATE ${static_target})
endfunction()


# MVP support microservices
qtrade_add_support_service(qtrade_config_service config_service)
qtrade_add_support_service(qtrade_account_service account_service)
qtrade_add_support_service(qtrade_account_risk_service account_risk_service)
qtrade_add_support_service(qtrade_log_service log_service)

# For qtrade_install.cmake: install to bin/
set(QTRADE_SERVICE_TARGETS
    qtrade_config_service
    qtrade_account_service
    qtrade_account_risk_service
    qtrade_log_service
)
