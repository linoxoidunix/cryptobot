
set (PROJECT_NAME EXAMPLE1)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
aot
# concurrentqueue::concurrentqueue
concurrentqueue
magic_enum
nlohmann_json::nlohmann_json
simdjson::simdjson
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

