cmake_minimum_required(VERSION 3.16)
project(OrderbookProject CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(OrderbookTest
  test.cpp
  ../src/OrderBook.cpp
)

target_include_directories(OrderbookTest PRIVATE ../include)
target_compile_definitions(OrderbookTest PRIVATE ORDERBOOK_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/TestFiles")
target_link_libraries(OrderbookTest gtest_main)

include(GoogleTest)
gtest_discover_tests(OrderbookTest)