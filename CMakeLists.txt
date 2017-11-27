cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
project(deskcycle)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

## Dependencies
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
include(cmake/BoostSML.cmake)

set(SOURCE_FILES
    main.cpp
    detector.cpp
    )

set(HEADER_FILES
    detector.hpp
    detector_fsm.hpp
    )

add_executable(deskcycle ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(deskcycle Threads::Threads ${SDL2_LIBRARY})
target_include_directories(deskcycle PRIVATE ${SDL2_INCLUDE_DIR} external/)

if (MSVC)
    set(MY_COMPILE_OPTIONS /W3 /FI ciso646)
else ()
    set(MY_COMPILE_OPTIONS -Wall -Wextra)
endif ()

target_compile_options(deskcycle PRIVATE ${MY_COMPILE_OPTIONS})
