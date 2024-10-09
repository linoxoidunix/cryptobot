
set (PROJECT_NAME bch_libuv_vs_boost_asio_vs_qt)
project(${PROJECT_NAME})

file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
benchmark::benchmark
uv
)

target_include_directories(${PROJECT_NAME}
PRIVATE
${CMAKE_SOURCE_DIR}
${Qt6_INCLUDE_DIRS} 
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
