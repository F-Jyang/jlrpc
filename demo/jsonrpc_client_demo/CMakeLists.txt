set(SRCS
    "./main.cpp"
)

add_executable(jsonrpc_client_demo ${SRCS})

target_include_directories(jsonrpc_client_demo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(jsonrpc_client_demo PUBLIC
    jlrpc_static
)
