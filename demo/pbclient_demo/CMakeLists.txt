
set(SRCS
    "./main.cpp"
    "./query_service_impl.h"
    "./query_service_impl.cpp"
)

add_executable(pbclient_demo ${SRCS})

target_include_directories(pbclient_demo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(pbclient_demo PUBLIC
    jlrpc_static
    protobuf_demo
)
