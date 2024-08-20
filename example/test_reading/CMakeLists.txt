
set (PROJECT_NAME test_reading)
project(${PROJECT_NAME})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

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
prometheus-cpp::core 
prometheus-cpp::util 
prometheus-cpp::civetweb 
prometheus-cpp::pull
magic_enum::magic_enum
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
