add_executable(woeful
  main.cpp
  server.cpp
  websocketManager.cpp
)

include(FetchContent)
FetchContent_Declare(
    cli11_proj
    QUIET
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG main
)

FetchContent_MakeAvailable(cli11_proj)

FetchContent_Declare(
    expected 
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
)
FetchContent_MakeAvailable(expected)

target_link_libraries(woeful PRIVATE usockets z CLI11::CLI11)

install(TARGETS woeful RUNTIME DESTINATION bin)

add_custom_command(TARGET woeful POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the src directory")

