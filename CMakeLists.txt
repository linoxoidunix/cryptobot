cmake_minimum_required(VERSION 3.29)
set (PROJECT_NAME aot)
project(${PROJECT_NAME} C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(OpenSSL)
add_subdirectory(clickhouse-cpp)
find_package(Threads REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED)
find_package(fast_double_parser REQUIRED)
add_subdirectory(googletest)
# add_subdirectory(benchmark)
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

find_package(benchmark)
find_package(Boost 1.65.0) 
find_package(Python COMPONENTS Interpreter Development)
find_package(concurrentqueue)
find_package(unordered_dense)

file(GLOB LIST_H
     "aot/*.h"
     "aot/common/*.h"
     "aot/market_data/*.h"
     "aot/strategy/*.h"
     "aot/python/*.h"
)
file(GLOB LIST_CPP
     "src/*.cpp"
)
set(ALL_FILES ${LIST_H} ${LIST_CPP})
message (${ALL_FILES})

set (PROJECT_NAME aot)
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
    magic_enum::magic_enum
    ${Boost_LIBRARIES}
    simdjson::simdjson
    nlohmann_json::nlohmann_json
    Python::Python
    concurrentqueue::concurrentqueue
    fast_double_parser::fast_double_parser
    unordered_dense::unordered_dense
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${CMAKE_CURRENT_LIST_DIR}/aot/python\"")

add_subdirectory(example)
add_subdirectory(test)

