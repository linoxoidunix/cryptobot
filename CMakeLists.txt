cmake_minimum_required(VERSION 3.29)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(OpenSSL)
add_subdirectory(clickhouse-cpp)
add_subdirectory(magic_enum)
add_subdirectory(simdjson)
add_subdirectory(googletest)
add_subdirectory(benchmark)
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.65.0) 

file(GLOB LIST_H
     "bybit/*.h"
)
file(GLOB LIST_CPP
     "src/*.cpp"
)
set(ALL_FILES ${LIST_H} ${LIST_CPP})
message (${ALL_FILES})

set (PROJECT_NAME bybit)
project(${PROJECT_NAME})


file(GLOB SRC
     "src/*.cpp"
)

#set(CMAKE_CXX_CLANG_TIDY clang-tidy ${ALL_FILES}, -header-filter=, -checks=cppcoreguidlines-*, -warnings-as-errors=*, -p build)

add_library (${PROJECT_NAME}
    ${SRC}
)

target_include_directories(
    ${PROJECT_NAME}
    ${BOOST_BEAST_FILES}
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    boost_coroutine
    boost_chrono
    boost_thread
    boost_system
    OpenSSL::SSL OpenSSL::Crypto
    magic_enum
    ${Boost_LIBRARIES}
    simdjson::simdjson
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

add_subdirectory(example)
add_subdirectory(test)

