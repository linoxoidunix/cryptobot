
set (PROJECT_NAME cpp_with_python)
project(${PROJECT_NAME})

find_package(Python COMPONENTS Interpreter Development)
message("Python_FOUND:${Python_FOUND}")

file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
bybit
Python::Python
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${CMAKE_CURRENT_LIST_DIR}\"")
