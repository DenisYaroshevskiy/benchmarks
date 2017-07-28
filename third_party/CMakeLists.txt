set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/third_party/cmake
    )

include(ConfigSafeGuards)

set(BENCHMARK_ENABLE_TESTING OFF)

add_subdirectory(catch)

include(ConfigGBench)
