set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 2.8.12)
project(zip)

add_definitions("-std=c++14")
aux_source_directory(src SRC)

add_executable(zip ${SRC})
target_link_libraries(zip )

