
set (PROJECT_NAME cpp_with_python)
project(${PROJECT_NAME})

find_package(Python COMPONENTS Interpreter Development)
message("Python_FOUND:${Python_FOUND}")
message("Python_VERSION:${Python_VERSION}")
message("Python_Development_FOUND:${Python_Development_FOUND}")
message("Python_LIBRARIES:${Python_LIBRARIES}")
message("Python_INCLUDE_DIRS:${Python_INCLUDE_DIRS}")

include_directories(${Python_INCLUDE_DIRS})
list(APPEND LINKLIBS ${Python_LIBRARIES})

file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
bybit
${LINKLIBS}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

set (PYTHON_PATH ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE "PYTHON_PATH=\"${PYTHON_PATH}\"")
