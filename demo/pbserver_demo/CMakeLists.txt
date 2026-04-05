
set(SRCS
    "./query_service_impl.h"
    "./query_service_impl.cpp"
    "./main.cpp"
)

add_executable(pbserver_demo ${SRCS})

target_include_directories(pbserver_demo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(pbserver_demo PUBLIC
    jlrpc_static
    protobuf_demo
)
