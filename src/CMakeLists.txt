find_package(protobuf CONFIG QUIET)
if(NOT protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
aux_source_directory("./net" NET_SRC)
aux_source_directory("./net/pb" NET_SRC)
# aux_source_directory("./net/json_rpc" NET_SRC)
aux_source_directory("./rpc" RPC_SRC)

list(APPEND SRCS ${NET_SRC} ${RPC_SRC})

add_library(jlrpc_static STATIC
    ${SRCS}
)

target_include_directories(jlrpc_static PUBLIC
    "${3rd}/asio/asio/include"
    "${3rd}/nlohmann_json/single_include/"
    "${3rd}/httplib/"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

# 根据找到的包配置链接库
if(TARGET protobuf::libprotobuf)
    target_link_libraries(jlrpc_static PUBLIC protobuf::libprotobuf)
else()
    target_link_libraries(jlrpc_static PUBLIC ${Protobuf_LIBRARIES})
    target_include_directories(jlrpc_static PUBLIC ${Protobuf_INCLUDE_DIRS})
endif()
