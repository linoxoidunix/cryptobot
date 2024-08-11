
set (PROJECT_NAME prometheus_cpp)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
#aot
prometheus-cpp::core 
prometheus-cpp::util 
prometheus-cpp::civetweb 
prometheus-cpp::pull
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

