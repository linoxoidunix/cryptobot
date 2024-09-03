
set (PROJECT_NAME unit_tests)
project(${PROJECT_NAME})


add_executable (${PROJECT_NAME}_trading_pair
    ${CMAKE_CURRENT_LIST_DIR}/TradingPair.cpp
)

target_link_libraries(${PROJECT_NAME}_trading_pair
aot
gtest_main
concurrentqueue::concurrentqueue
)

target_include_directories(${PROJECT_NAME}_trading_pair
PRIVATE
${CMAKE_SOURCE_DIR}
)




set_property(TARGET ${PROJECT_NAME}_trading_pair PROPERTY CXX_STANDARD 23)
