
set (PROJECT_NAME bch_json_reader)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
simdjson::simdjson
benchmark::benchmark
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)