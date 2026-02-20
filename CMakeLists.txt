cmake_minimum_required(VERSION 3.10)
project(web_server CXX)

set(CMAKE_CXX_STANDARD 17)

# Include the headers directory
include_directories(inc)

# Define the executable and its source files
add_executable(web_server 
    src/main.cpp 
    src/parser.cpp 
    src/router.cpp
    src/template.cpp
    src/script_lex.cpp
    src/logic_engine.cpp
    src/script_ast.cpp
    src/script_parser.cpp
)

# Create a symlink of the service directory in the build directory
add_custom_command(TARGET web_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink 
    ${CMAKE_CURRENT_SOURCE_DIR}/service 
    ${CMAKE_CURRENT_BINARY_DIR}/service
)