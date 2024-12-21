
set (PROJECT_NAME unit_test_binance_local_test)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
aot
gtest_main
gmock_main
concurrentqueue
magic_enum::magic_enum
tomlplusplus::tomlplusplus
nlohmann_json::nlohmann_json
simdjson::simdjson
)

target_include_directories(${PROJECT_NAME}
PRIVATE
${CMAKE_SOURCE_DIR}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)