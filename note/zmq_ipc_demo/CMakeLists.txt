cmake_minimum_required(VERSION 3.10)
project(zmq_ipc_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(zmq_ipc_server zmq_ipc_server.cpp)
target_include_directories(zmq_ipc_server PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(zmq_ipc_server ${ZMQ_LIBRARIES})

add_executable(zmq_ipc_client zmq_ipc_client.cpp)
target_include_directories(zmq_ipc_client PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(zmq_ipc_client ${ZMQ_LIBRARIES})
