find_package(Protobuf CONFIG REQUIRED)

message(STATUS "protobuf version: ${Protobuf_VERSION}")

if(TARGET protobuf::libprotobuf)
    message(STATUS "find protobuf::libprotobuf")
endif()

set(proto_file query.proto)

add_library(protobuf_demo ${proto_file})

protobuf_generate(TARGET protobuf_demo LANGUAGE cpp)

target_include_directories(protobuf_demo PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(protobuf_demo PUBLIC
    protobuf::libprotobuf
)

