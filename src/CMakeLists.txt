add_executable(woefull
  main.cpp
  server.cpp
  websocketManager.cpp
)

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    expected 
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
)
FetchContent_MakeAvailable(expected)

FetchContent_Declare(
    mdns 
    GIT_REPOSITORY https://github.com/mjansson/mdns.git
    GIT_TAG main
)
FetchContent_MakeAvailable(mdns)

target_link_libraries(woefull argparse usockets z)

add_custom_command(TARGET woefull POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the src directory")

