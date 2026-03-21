cmake_minimum_required(VERSION 3.16)
project(qitto VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define LINUX_PORT for all targets
add_compile_definitions(LINUX_PORT)

# --- Phase 0: CRC32 validation test ---
add_executable(test_crc32
    tests/test_crc32.cpp
    src/Crc32Dynamic.cpp
)
target_include_directories(test_crc32 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

enable_testing()
add_test(NAME test_crc32 COMMAND test_crc32)
