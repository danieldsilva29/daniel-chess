cmake_minimum_required(VERSION 3.13.4)
project(DanielChess)

file(GLOB_RECURSE srcs ${CMAKE_SOURCE_DIR}/src/*.cpp)

set(NAME danielchess)

add_executable(${NAME} ${srcs})
target_compile_options(${NAME} PUBLIC "-fno-exceptions")
target_compile_options(${NAME} PUBLIC "-fno-rtti")
target_compile_options(${NAME} PUBLIC "-march=native")
target_compile_options(${NAME} PUBLIC "-O3")

target_compile_options(${NAME} PUBLIC "-g")



target_include_directories(${NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_property(TARGET danielchess PROPERTY CXX_STANDARD 17)
