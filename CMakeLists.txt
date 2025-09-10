cmake_minimum_required(VERSION 3.16)

project(whatmade
        VERSION 0.2.0
        HOMEPAGE_URL "https://github.com/ANGulchenko/whatmade"
        DESCRIPTION "Whatmade is a Linux daemon that monitors user-specified directories and records which process created each file.")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_LIBRARY(AnyOption STATIC
    AnyOption/anyoption.cpp AnyOption/anyoption.h
)

ADD_LIBRARY(Imp STATIC
    Imp/Imp.cpp Imp/Imp.h
)

ADD_LIBRARY(Log STATIC
    Log/logger.cpp Log/logger.h
)

add_executable(whatmade main.cpp
    os_specific.h
    xattr.h xattr.cpp
    configsingleton.h configsingleton.cpp)

target_link_libraries(whatmade AnyOption Imp Log config++)

