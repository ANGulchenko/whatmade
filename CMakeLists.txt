cmake_minimum_required(VERSION 3.28)

project(whomade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_LIBRARY(AnyOption STATIC
    AnyOption/anyoption.cpp AnyOption/anyoption.h
)

ADD_LIBRARY(Imp STATIC
    Imp/Imp.cpp Imp/Imp.h
)

add_executable(whomade main.cpp
    os_specific.h
    sqlite_specific.h
    Log/logger.h
    README.md CHANGELOG.md)

target_link_libraries(whomade AnyOption Imp sqlite3)

