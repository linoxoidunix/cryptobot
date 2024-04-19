cmake_minimum_required(VERSION 3.29)
find_package(OpenSSL)
set (PROJECT_NAME bybit)
project(${PROJECT_NAME})


file(GLOB SRC
     "src/*.cpp"
)


add_library (${PROJECT_NAME}
    ${SRC}
)

target_include_directories(
    ${PROJECT_NAME}
    ${BOOST_BEAST_FILES}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    boost_coroutine
    boost_chrono
    boost_thread
    boost_system
    OpenSSL::SSL OpenSSL::Crypto
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

add_subdirectory(example)
add_subdirectory(clickhouse-cpp)
