cmake_minimum_required(VERSION 3.29)
set (PROJECT_NAME aot)
project(${PROJECT_NAME} C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(OpenSSL)
#add_subdirectory(clickhouse-cpp)
add_subdirectory(aot_data)
find_package(Threads REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED)
find_package(fast_double_parser REQUIRED)
find_package(prometheus-cpp REQUIRED)
find_package(re2 REQUIRED)
find_package(absl REQUIRED)#re2 need in absl
add_subdirectory(googletest)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)#ignore tests for benchmark
add_subdirectory(benchmark)
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

# option(TURN_ON_LATENCY_FOR_GENERATOR_BID_ASK_SERVICE "turn on measure latency for generator bid ask" true)
# option(TURN_ON_LATENCY_FOR_TRADE_ENGINE "turn on measure latency for trade engine" true)


find_package(benchmark)
find_package(Boost 1.65.0) 
find_package(Python COMPONENTS Interpreter Development)
find_package(concurrentqueue)
find_package(unordered_dense)

                                                    # include(ExternalProject)
                                                    # ExternalProject_Add(cpptrader
                                                    #   GIT_REPOSITORY    https://github.com/chronoxor/CppTrader.git
                                                    #   PREFIX CppTrader
                                                    # #   EXCLUDE_FROM_ALL TRUE
                                                    # #   SOURCE_DIR   = CppTrader/source

                                                    #   CMAKE_ARGS
                                                    #     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                                    #     -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                                    #     -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
                                                    # )

                                                    # ExternalProject_Get_Property(cpptrader install_dir)
file(GLOB LIST_H
     "aot/*.h"
     "aot/common/*.h"
     "aot/market_data/*.h"
     "aot/order_gw/*.h"
     "aot/prometheus/*.h"
     "aot/strategy/*.h"
     "aot/python/*.h"
)
file(GLOB LIST_CPP
     "src/*.cpp"
)
set(ALL_FILES ${LIST_H} ${LIST_CPP})
message (${LIST_CPP})

set (PROJECT_NAME aot)
project(${PROJECT_NAME})


file(GLOB SRC
     "src/*.cpp"
)

#set(CMAKE_CXX_CLANG_TIDY clang-tidy ${ALL_FILES}, -header-filter=, -checks=cppcoreguidlines-*, -warnings-as-errors=*, -p build)

add_library (${PROJECT_NAME}
    ${SRC}
)

#add_library(cpp_trader STATIC IMPORTED)
#SET_TARGET_PROPERTIES(cpp_trader PROPERTIES IMPORTED_LOCATION /home/linoxoidunix/Programming/cplusplus/cryptobot/sdk/cpptrader/lib/libcpptrader.a)

target_include_directories(
    ${PROJECT_NAME}
    ${BOOST_BEAST_FILES}
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    #/home/linoxoidunix/Programming/cplusplus/cryptobot/sdk/cpptrader/include
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
    prometheus-cpp::core 
    prometheus-cpp::util 
    prometheus-cpp::civetweb 
    prometheus-cpp::pull
    #cpp_trader
    absl::base
    re2::re2
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${CMAKE_CURRENT_LIST_DIR}/aot/python\"")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE=false")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_TRADE_ENGINE=false")


add_subdirectory(example)
add_subdirectory(tests)

