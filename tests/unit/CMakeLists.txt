
set (PROJECT_NAME unit_tests)
project(${PROJECT_NAME})


add_executable (${PROJECT_NAME}_trading_pair
    ${CMAKE_CURRENT_LIST_DIR}/TradingPair.cpp
)

target_link_libraries(${PROJECT_NAME}_trading_pair
aot
gtest_main
# concurrentqueue::concurrentqueue
concurrentqueue
)

target_include_directories(${PROJECT_NAME}_trading_pair
PRIVATE
${CMAKE_SOURCE_DIR}
)

add_executable (${PROJECT_NAME}_config_back_testing
    ${CMAKE_CURRENT_LIST_DIR}/ConfigBackTesting.cpp
)

target_link_libraries(${PROJECT_NAME}_config_back_testing
aot
gtest_main
# concurrentqueue::concurrentqueue
concurrentqueue
tomlplusplus::tomlplusplus
)

target_include_directories(${PROJECT_NAME}_config_back_testing
PRIVATE
${CMAKE_SOURCE_DIR}
)




set_property(TARGET ${PROJECT_NAME}_trading_pair PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME}_config_back_testing PROPERTY CXX_STANDARD 23)
