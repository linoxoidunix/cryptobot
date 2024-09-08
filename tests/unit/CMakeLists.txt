
set (PROJECT_NAME unit_test)
project(${PROJECT_NAME})

macro(cxx_executable_extended prjname src)
  set(prj ${PROJECT_NAME}_${prjname})
  add_executable(${prj} ${src})
  foreach (lib "${ARGN}")
    target_link_libraries(${prj} ${lib})
  endforeach()
  target_include_directories(${prj} PRIVATE ${CMAKE_SOURCE_DIR})
  set_property(TARGET ${prj} PROPERTY CXX_STANDARD 23)
  gtest_add_tests(TARGET ${prj})
endmacro()

macro(cxx_executable name dir libs)
cxx_executable_extended(${name} ${dir}/${name}.cc ${libs}  ${ARGN})
endmacro()

cxx_executable(trading_pair ${CMAKE_CURRENT_LIST_DIR} aot gtest_main concurrentqueue)
cxx_executable(config_backtesting ${CMAKE_CURRENT_LIST_DIR} aot gtest_main tomlplusplus::tomlplusplus)
cxx_executable(orderbook ${CMAKE_CURRENT_LIST_DIR} aot concurrentqueue gtest_main)
cxx_executable(order_manager_backtesting ${CMAKE_CURRENT_LIST_DIR} aot concurrentqueue unordered_dense::unordered_dense gtest_main)
cxx_executable(cross_arbitrage ${CMAKE_CURRENT_LIST_DIR} aot concurrentqueue gtest_main)