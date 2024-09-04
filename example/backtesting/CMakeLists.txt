
set (PROJECT_NAME backtesting)
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
# concurrentqueue::concurrentqueue
concurrentqueue
Python::Python
${Boost_LIBRARIES}
unordered_dense::unordered_dense
prometheus-cpp::core 
prometheus-cpp::util 
prometheus-cpp::civetweb 
prometheus-cpp::pull
magic_enum::magic_enum
tomlplusplus::tomlplusplus
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
# target_compile_definitions(${PROJECT_NAME} PRIVATE "CURRENT_PATH=\"${CMAKE_CURRENT_LIST_DIR}\"")
configure_file(${CMAKE_SOURCE_DIR}/aot_data/launch_config/back_testing.toml ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_SOURCE_DIR}/aot_data/ohlcv_history/ohlcv.csv ${CMAKE_CURRENT_BINARY_DIR})