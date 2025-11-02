cmake_minimum_required(VERSION 3.15)
project(habr_parser C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(habr_parser
    src/main.c
    src/http.c
    src/html_scan.c
    src/extractor.c
    src/csv_writer.c
    src/entities.c
    src/utils.c
)

find_package(CURL REQUIRED)

target_include_directories(habr_parser PRIVATE src)

target_link_libraries(habr_parser PRIVATE CURL::libcurl)

if(MSVC)
    target_compile_options(habr_parser PRIVATE /W4 /WX)
else()
    target_compile_options(habr_parser PRIVATE -Wall -Wextra -Werror -O2)
endif()
