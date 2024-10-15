cmake_minimum_required(VERSION 3.28)
set (PROJECT_NAME aot)
project(${PROJECT_NAME} C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(OpenSSL)
#add_subdirectory(clickhouse-cpp)
add_subdirectory(aot_data)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
#find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)
include(FetchContent)

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
)
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
)

SET(ENABLE_TESTING OFF)
FetchContent_Declare(
    prometheus-cpp
    GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
    FIND_PACKAGE_ARGS NAMES prometheus-cpp
)

set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG        origin/master
    OVERRIDE_FIND_PACKAGE
)
 
FetchContent_Declare(
    re2
    GIT_REPOSITORY https://github.com/google/re2.git
    GIT_TAG        main
    # OVERRIDE_FIND_PACKAGE
)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)


FetchContent_MakeAvailable( simdjson magic_enum nlohmann_json prometheus-cpp tomlplusplus)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG main
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)#ignore tests for benchmark
FetchContent_MakeAvailable(benchmark)

FetchContent_Declare(gtest_main
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
)
FetchContent_MakeAvailable(gtest_main)

FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
    GIT_TAG        master
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(concurrentqueue)

FetchContent_Declare(
    unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(unordered_dense)


FetchContent_MakeAvailable(abseil)
FetchContent_MakeAvailable(re2)

#FetchContent_Declare(
#    boost
#    GIT_REPOSITORY https://github.com/boostorg/boost.git
#    GIT_TAG        boost-1.83.0  # Change to the desired Boost version
#)

#FetchContent_MakeAvailable(boost)

#include_directories(${boost_SOURCE_DIR})


# find_package(Boost 1.83.0 REQUIRED) 
find_package(Python COMPONENTS Interpreter Development)
find_package(concurrentqueue)

file(GLOB LIST_H
     "aot/*.h"
     "aot/bus/*.h"
     "aot/common/*.h"
     "aot/market_data/*.h"
     "aot/order_gw/*.h"
     "aot/prometheus/*.h"
     "aot/strategy/*.h"
     "aot/strategy/cross_arbitrage/*.h"
     "aot/third_party/*.h"
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
    ${LIST_H} #for use emhash library need add LIST_H
    ${SRC}
)

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
    #magic_enum::magic_enum
    magic_enum
    ${Boost_LIBRARIES}
    simdjson::simdjson
    nlohmann_json::nlohmann_json
    Python::Python
    concurrentqueue
    unordered_dense::unordered_dense
    prometheus-cpp::core 
    prometheus-cpp::util 
    prometheus-cpp::civetweb 
    prometheus-cpp::pull
    #cpp_trader
    absl::base
    re2::re2
    tomlplusplus::tomlplusplus
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_compile_definitions(${PROJECT_NAME} PRIVATE "ROOT_PATH=\"${CMAKE_CURRENT_LIST_DIR}\"")
target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${CMAKE_CURRENT_LIST_DIR}/aot/python\"")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE=false")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_TRADE_ENGINE=false")

add_subdirectory(example)
enable_testing()
include(GoogleTest)

add_subdirectory(tests)

