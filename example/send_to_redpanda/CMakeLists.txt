
set (PROJECT_NAME produce_message_to_redpanda)
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
concurrentqueue
Python::Python
${Boost_LIBRARIES}
unordered_dense::unordered_dense
prometheus-cpp::core 
prometheus-cpp::util 
prometheus-cpp::civetweb 
prometheus-cpp::pull
magic_enum
tomlplusplus::tomlplusplus
simdjson::simdjson
rdkafka
nlohmann_json::nlohmann_json
cppkafka
protobuf
)

#cpp kafka wrong default include, i need add string below for coirrect working
target_include_directories(${PROJECT_NAME} PRIVATE ${cppkafka_SOURCE_DIR}/include)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)