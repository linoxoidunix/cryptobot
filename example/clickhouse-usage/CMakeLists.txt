cmake_minimum_required(VERSION 3.12)
project(clickhouse-example)

add_executable(${PROJECT_NAME} "clickhouse.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE clickhouse-cpp-lib)
