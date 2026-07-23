# ---------------------------------------------------------------------------
# qtrade_proto: Protobuf / gRPC code generation and static library
#
# Flow (configure time):
#   1. GLOB all .proto under proto/ (flat layout, e.g. proto/config/v1/*.proto)
#   2. Run protoc -> build/proto/config/v1/...
#   3. Stage *.h -> build/include/qtrade/proto/config/v1/... (public #include path)
#   4. GLOB build/proto/*.pb.cc and build qtrade_proto
#
# Public include stays: #include <qtrade/proto/config/v1/config.pb.h>
# ---------------------------------------------------------------------------

# --- Dependencies (pkg-config) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(QTRADE_GRPC REQUIRED grpc++)
pkg_check_modules(QTRADE_PROTOBUF REQUIRED protobuf)

# --- Code generation tools ---
find_program(QTRADE_PROTOC protoc REQUIRED)
find_program(QTRADE_GRPC_PLUGIN grpc_cpp_plugin REQUIRED)

# --- Paths ---
set(QTRADE_PROTO_ROOT ${CMAKE_SOURCE_DIR}/proto)
set(QTRADE_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto)
set(QTRADE_PROTO_PUBLIC_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)

# --- 1. Scan proto/ ---
file(GLOB_RECURSE QTRADE_PROTO_FILES CONFIGURE_DEPENDS "${QTRADE_PROTO_ROOT}/*.proto")

# --- Static library target ---
if(QTRADE_PROTO_FILES)
  set(QTRADE_PROTO_REL_FILES "")
  foreach(_proto ${QTRADE_PROTO_FILES})
    file(RELATIVE_PATH _rel ${QTRADE_PROTO_ROOT} ${_proto})
    list(APPEND QTRADE_PROTO_REL_FILES ${_rel})
  endforeach()

  # --- 2. Run protoc (clean gen dir first) ---
  file(REMOVE_RECURSE ${QTRADE_PROTO_GEN_DIR})
  file(MAKE_DIRECTORY ${QTRADE_PROTO_GEN_DIR})
  execute_process(
      COMMAND ${QTRADE_PROTOC}
          --proto_path=.
          --cpp_out=${QTRADE_PROTO_GEN_DIR}
          --grpc_out=${QTRADE_PROTO_GEN_DIR}
          --plugin=protoc-gen-grpc=${QTRADE_GRPC_PLUGIN}
          ${QTRADE_PROTO_REL_FILES}
      WORKING_DIRECTORY ${QTRADE_PROTO_ROOT}
      RESULT_VARIABLE _qtrade_protoc_code)
  if(NOT _qtrade_protoc_code EQUAL 0)
    message(FATAL_ERROR "protoc failed with exit code ${_qtrade_protoc_code}")
  endif()

  # --- 3. Stage headers under build/include/qtrade/proto/ ---
  file(REMOVE_RECURSE ${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}/qtrade/proto)
  file(MAKE_DIRECTORY ${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}/qtrade/proto)
  file(GLOB _proto_gen_children RELATIVE ${QTRADE_PROTO_GEN_DIR} ${QTRADE_PROTO_GEN_DIR}/*)
  foreach(_child ${_proto_gen_children})
    if(IS_DIRECTORY ${QTRADE_PROTO_GEN_DIR}/${_child})
      file(COPY ${QTRADE_PROTO_GEN_DIR}/${_child}
           DESTINATION ${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}/qtrade/proto)

      # Rewrite internal includes so staged headers work with -I build/include
      file(GLOB_RECURSE _staged_headers
           "${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}/qtrade/proto/${_child}/*.h")
      foreach(_hdr ${_staged_headers})
        file(READ ${_hdr} _hdr_content)
        foreach(_include_child ${_proto_gen_children})
          string(REPLACE "#include \"${_include_child}/"
                         "#include \"qtrade/proto/${_include_child}/"
                         _hdr_content "${_hdr_content}")
        endforeach()
        file(WRITE ${_hdr} "${_hdr_content}")
      endforeach()
    endif()
  endforeach()

  # --- 4. Collect compile units from raw protoc output ---
  file(GLOB_RECURSE QTRADE_PROTO_SRCS CONFIGURE_DEPENDS
      "${QTRADE_PROTO_GEN_DIR}/*.pb.cc"
      "${QTRADE_PROTO_GEN_DIR}/*.grpc.pb.cc")

  add_library(qtrade_proto STATIC ${QTRADE_PROTO_SRCS})
  target_compile_options(qtrade_proto PRIVATE ${QTRADE_GRPC_CFLAGS_OTHER})

  # Public:  #include <qtrade/proto/config/v1/config.pb.h>
  # Private: compile .pb.cc which #include "config/v1/..." relative to build/proto/
  target_include_directories(qtrade_proto
    PUBLIC
      $<BUILD_INTERFACE:${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
      ${QTRADE_PROTO_GEN_DIR}
  )
else()
  message(WARNING "No .proto files under ${QTRADE_PROTO_ROOT}; qtrade_proto is an empty INTERFACE target")
  add_library(qtrade_proto INTERFACE)
  target_include_directories(qtrade_proto INTERFACE
    $<BUILD_INTERFACE:${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
endif()

# Third-party headers required by generated *.pb.h / *.grpc.pb.h (e.g. <grpcpp/...>,
# <google/protobuf/...>). SYSTEM suppresses warnings from those headers.
# PUBLIC propagates to qtrade_common / qtrade_core so dependents compile without
# calling pkg_check_modules(grpc++) themselves.
target_include_directories(qtrade_proto SYSTEM PUBLIC
    ${QTRADE_GRPC_INCLUDE_DIRS}
    ${QTRADE_PROTOBUF_INCLUDE_DIRS})

# Link grpc++ and protobuf runtime. PUBLIC propagates link requirements to any
# target that links qtrade_proto (directly or via qtrade_common / qtrade_core).
target_link_libraries(qtrade_proto PUBLIC
    ${QTRADE_GRPC_LIBRARIES}
    ${QTRADE_PROTOBUF_LIBRARIES})
