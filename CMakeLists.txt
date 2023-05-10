cmake_minimum_required(VERSION 3.20)

project(physram C)

set(CMAKE_C_STANDARD 11)

set(WIN32_UNICODE true)

set(WIN32_SUBSYS_WIN false)

if (WIN32 AND MINGW)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -Wno-unused-parameter ")

        # -mconsole: console subsystem
        # -mwindows: window subsystem
        if (WIN32_SUBSYS_WIN)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindow -DSUBSYS_WINDOW ")
        else()
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mconsole ")
        endif()

        if (WIN32_UNICODE)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -municode -DUNICODE -D_UNICODE ")
        endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra ")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -ggdb3 -gdwarf-4 -O0 ")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ")
endif()

add_subdirectory(libinpoutx64)

add_executable(physram
               main.c
               logging.c
               logging.h
               getopt.c
               getopt.h
               utils.h
               )

target_link_libraries(physram ntdll)
target_link_libraries(physram ntoskrnl)
target_link_libraries(physram psapi)
target_link_libraries(physram inpoutx64)