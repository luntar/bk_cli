cmake_minimum_required(VERSION 3.16)

project(bk_cli LANGUAGES CXX)

add_library(bk_cli_lib
    Command.cpp
    Command_Helpers.cpp
    Console_Line_Editor.cpp
)

target_include_directories(bk_cli_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(bk_cli_lib PUBLIC cxx_std_17)

add_executable(bk_cli main.cpp)
target_link_libraries(bk_cli PRIVATE bk_cli_lib)

target_compile_features(bk_cli PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(bk_cli_lib PRIVATE /W4 /permissive-)
    target_compile_options(bk_cli PRIVATE /W4 /permissive-)
else()
    target_compile_options(bk_cli_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(bk_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()
