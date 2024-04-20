cmake_minimum_required(VERSION 3.29)
find_package(OpenSSL)
add_subdirectory(clickhouse-cpp)
add_subdirectory(magic_enum)
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.65.0) 

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
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    boost_coroutine
    boost_chrono
    boost_thread
    boost_system
    OpenSSL::SSL OpenSSL::Crypto
	magic_enum
    ${Boost_LIBRARIES}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

add_subdirectory(example)

