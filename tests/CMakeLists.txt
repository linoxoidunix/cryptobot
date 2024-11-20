add_subdirectory(perfomance)
add_subdirectory(unit)
option(LOCAL_TEST "Enable local-only tests" OFF)

if (LOCAL_TEST)
    add_subdirectory(local_tests)
endif()