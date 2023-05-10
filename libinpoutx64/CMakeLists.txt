cmake_minimum_required(VERSION 3.20)

set(LIB_NAME inpoutx64)

project(${LIB_NAME} C)

set(CMAKE_C_STANDARD 11)

add_library(${LIB_NAME} inpoutx64.c inpoutx64.h)

if (WIN32 AND MINGW)
        target_compile_options(${LIB_NAME} PRIVATE -municode)
        target_compile_definitions(${LIB_NAME} PRIVATE -DUNICODE)
        target_compile_definitions(${LIB_NAME} PRIVATE -D_UNICODE)
        target_link_options(${LIB_NAME} PRIVATE -municode)
endif()

target_compile_options(${LIB_NAME} PRIVATE -Wall)
target_compile_options(${LIB_NAME} PRIVATE -Wextra)
# target_compile_options(${LIB_NAME} PRIVATE -Wno-unused-parameter)
# target_compile_options(${LIB_NAME} PRIVATE -Wno-unused-function)
