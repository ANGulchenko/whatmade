cmake_minimum_required(VERSION 3.28)

project(whomade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_LIBRARY(AnyOption STATIC
    AnyOption/anyoption.cpp AnyOption/anyoption.h
)

add_executable(whomade main.cpp
    os_specific.h
    sqlite_specific.h)

target_link_libraries(whomade AnyOption sqlite3)

