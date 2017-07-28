project(benchmarks)

set(SOURCE_EXE
#  bit_operations.h
#  copy.h
#  flat_set_insert_benchmark.cc
#  insert_algorithms.h
#  list_benchmark.cc
  nth_element_benchmark.cc
#  singular_insert.cc
#  unique_ptr_set_benchmark.cc
)

add_executable(benchmarks ${SOURCE_EXE})

target_link_libraries(benchmarks benchmark)
