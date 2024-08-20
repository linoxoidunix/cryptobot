
set (PROJECT_NAME market_order_book)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
aot
benchmark::benchmark
concurrentqueue::concurrentqueue
magic_enum::magic_enum
)

target_include_directories(${PROJECT_NAME}
PRIVATE
${CMAKE_SOURCE_DIR}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
target_compile_definitions(${PROJECT_NAME} PUBLIC "PATH_TO_DATA=\"${CMAKE_SOURCE_DIR}/aot_data\"")
