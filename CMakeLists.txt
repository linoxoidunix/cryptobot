cmake_minimum_required(VERSION 3.28)
set (PROJECT_NAME aot)
project(${PROJECT_NAME} C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     message(STATUS "Configuring ${PROJECT_NAME} for GNU or Clang with modules and debug info.")
#     add_compile_options(-fmodules-ts)  # Enable modules support for GCC/Clang
# elseif(MSVC)
#     message(STATUS "Configuring ${PROJECT_NAME} for MSVC with modules and debug info.")
#     add_compile_options(/experimental:module)  # Enable modules support for MSVC
# endif()

#set(CMAKE_CXX_EXTENSIONS FALSE)
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)
#set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

#for valgrind
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
# SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
# SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

find_package(OpenSSL)

#add_subdirectory(clickhouse-cpp)

add_subdirectory(aot_data)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

#find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)
message(STATUS "Starting FetchContent for simdjson...")
include(FetchContent)

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        master
)
message(STATUS "Declared simdjson FetchContent.")
FetchContent_MakeAvailable(simdjson)
message(STATUS "simdjson FetchContent is available.")
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        master
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



FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)


FetchContent_MakeAvailable(magic_enum nlohmann_json prometheus-cpp tomlplusplus)


# # Find rdkafka (ensure it is installed on your system)
# find_package(RdKafka REQUIRED)


# Fetch cppkafka
FetchContent_Declare(
    cppkafka
    GIT_REPOSITORY https://github.com/mfontanini/cppkafka
    GIT_TAG master
)
FetchContent_MakeAvailable(cppkafka)

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


find_package(Boost 1.87.0 REQUIRED) 

if (Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Boost not found!")
endif()

find_package(Python COMPONENTS Interpreter Development)
#find_package(concurrentqueue)
find_package(Protobuf REQUIRED)

file(GLOB LIST_H
     "aot/*.h"
     "aot/bus/*.h"
     "aot/common/*.h"
     "aot/market_data/*.h"
     "aot/order_gw/*.h"
     "aot/prometheus/*.h"
     "aot/proto/classes/*.h"
     "aot/strategy/*.h"
     "aot/pnl/*.h"
     "aot/strategy/arbitrage/*.h"
     "aot/strategy/cross_arbitrage/*.h"
     "aot/third_party/*.h"
     "aot/python/*.h"
)
file(GLOB LIST_CPP
     "src/*.cpp"
)

#for support modules
# file(GLOB LIST_CXX
#     "aot/*.cxx"
#     "aot/bus/*.cxx"
#     "aot/common/*.cxx"
#     "aot/market_data/*.cxx"
#     "aot/order_gw/*.cxx"
#     "aot/prometheus/*.cxx"
#     "aot/strategy/*.cxx"
#     "aot/strategy/cross_arbitrage/*.cxx"
#     "aot/third_party/*.cxx"
#     "aot/python/*.cxx"
# )

# Replace semicolons with newlines for better readability
string(REPLACE ";" "\n" LIST_CPP_WITH_NEWLINES "${LIST_CPP}")

# Display the formatted list
message(STATUS "Source files:\n${LIST_CPP_WITH_NEWLINES}")

set (PROJECT_NAME aot)
project(${PROJECT_NAME})


#file(GLOB SRC
#     "src/*.cpp"
#)
#set(ALL_FILES ${LIST_H} ${LIST_CPP})
#set(CMAKE_CXX_CLANG_TIDY clang-tidy ${ALL_FILES}, -header-filter=, -checks=cppcoreguidlines-*, -warnings-as-errors=*, -p build)

file(GLOB_RECURSE PROTO_FILES
     "aot/proto/*.proto"
)

# Укажите каталоги для вывода сгенерированных файлов
set(PROTO_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated")
set(PROTO_INCLUDE_DIR "${PROTO_OUTPUT_DIR}/include")
set(PROTO_SRC_DIR "${PROTO_OUTPUT_DIR}/src")

# Создайте папки для сгенерированных файлов
file(MAKE_DIRECTORY ${PROTO_INCLUDE_DIR})
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

string(REPLACE ";" "\n" PROTO_FILES_NEWLINES "${PROTO_FILES}")
message(STATUS "Find .proto files:\n${PROTO_FILES_NEWLINES}")

# Генерация исходных файлов C++ из .proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Логирование с переносами строк для читаемости списка файлов
string(REPLACE ";" "\n" PROTO_HDRS_NEWLINES "${PROTO_HDRS}")
string(REPLACE ";" "\n" PROTO_SRCS_NEWLINES "${PROTO_SRCS}")

# Печать списка заголовочных и исходных файлов
message(STATUS "Generated Proto Headers:\n${PROTO_HDRS_NEWLINES}")
message(STATUS "Generated Proto Sources:\n${PROTO_SRCS_NEWLINES}")

add_library (${PROJECT_NAME}
    ${LIST_H} #for use emhash library need add LIST_H
    ${LIST_CPP}
    ${PROTO_SRCS}
    # ${PROTO_HDRS}
)
#for support modules
# target_sources(${PROJECT_NAME} PUBLIC
#     FILE_SET all_my_modules TYPE CXX_MODULES FILES
#     ${LIST_CXX}
# )



# file(GLOB_RECURSE PROTO_FILES "${PROTO_DIR}/*.proto")

# Include directories
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/generated # Используется только во время сборки
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}  # Папка с сгенерированными .pb.h файлами
    PUBLIC
        # $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated> # Для сборки
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # Доступ к .pb.h при сборке
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # Доступ к aot
        $<INSTALL_INTERFACE:include> # Для установки
)


# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenSSL::SSL OpenSSL::Crypto
        magic_enum
        simdjson::simdjson
        nlohmann_json::nlohmann_json
        Python::Python
        concurrentqueue
        unordered_dense::unordered_dense
        prometheus-cpp::core 
        prometheus-cpp::util 
        prometheus-cpp::civetweb 
        prometheus-cpp::pull
        absl::base
        re2::re2
        tomlplusplus::tomlplusplus
        protobuf
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_compile_definitions(${PROJECT_NAME} PRIVATE "ROOT_PATH=\"${CMAKE_CURRENT_LIST_DIR}\"")
target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${CMAKE_CURRENT_LIST_DIR}/aot/python\"")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE=false")
target_compile_definitions(${PROJECT_NAME} PUBLIC "MEASURE_T_FOR_TRADE_ENGINE=false")

add_subdirectory(example)
add_subdirectory(sandbox_algo)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
else()
    message(STATUS "Tests are skipped.")
endif()

#if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # enable_testing()
    # include(GoogleTest)
    # add_subdirectory(tests)
#else()
#    message(STATUS "Tests will not be built in ${CMAKE_BUILD_TYPE} mode")
#endif()

# Installation rules
install(TARGETS ${PROJECT_NAME} magic_enum nlohmann_json unordered_dense tomlplusplus_tomlplusplus
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY aot/ DESTINATION include)
install(FILES ${PROTO_HDRS} DESTINATION include/generated)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Config.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)