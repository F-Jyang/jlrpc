cmake_minimum_required(VERSION 3.23.0)
project(jlrpc VERSION 0.1.0 LANGUAGES C CXX)

if(MSVC)
  add_compile_options("/utf-8")
  #   # 使用静态运行时匹配 protobuf
  #   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  #   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

  #   # 移除可能存在的 /MD 选项
  #   string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  #   string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(3rd "${CMAKE_SOURCE_DIR}/3rd")

# # 阶段	关键字	执行时机	典型用途
# # PRE_BUILD	构建前	源文件编译前	生成代码、检查依赖
# # PRE_LINK	链接前	编译后、链接前	处理中间文件
# # POST_BUILD	构建后	目标生成后	复制文件、生成符号、打包

# # 条件编译：仅在 Release 时分离调试符号
# function(add_executable_with_debug name)
#     add_executable(${name} ${ARGN})

#     if(CMAKE_BUILD_TYPE STREQUAL "Release")
#         target_compile_options(${name} PRIVATE -g)

#         add_custom_command(TARGET ${name} POST_BUILD
#             COMMAND ${CMAKE_OBJCOPY} --only-keep-debug
#             $<TARGET_FILE:${name}>
#             $<TARGET_FILE:${name}>.debug
#         )

#         add_custom_command(TARGET ${name} POST_BUILD
#             COMMAND ${CMAKE_OBJCOPY} --strip-debug
#             $<TARGET_FILE:${name}>
#         )

#         add_custom_command(TARGET ${name} POST_BUILD
#             COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink # --add-gnu-debuglink 的作用在于添加 debug section 
#             $<TARGET_FILE:${name}>.debug
#             $<TARGET_FILE:${name}>
#         )
#     endif()
# endfunction()

add_subdirectory("./proto")
add_subdirectory("./src")
add_subdirectory("./demo")
