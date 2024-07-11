
set (PROJECT_NAME test_reading)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
aot
concurrentqueue::concurrentqueue
Python::Python
${Boost_LIBRARIES}
unordered_dense::unordered_dense
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)