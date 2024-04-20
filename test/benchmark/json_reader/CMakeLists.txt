
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

target_include_directories(${PROJECT_NAME}
PRIVATE
${CMAKE_SOURCE_DIR}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
target_compile_definitions(${PROJECT_NAME} PRIVATE "PATH_TO_TEST=\"${CMAKE_CURRENT_LIST_DIR}\"")