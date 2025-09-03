cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# testing encryption algorithms for my own sanity
project(encryption-testing)

find_package(GCrypt REQUIRED)
find_package(Nettle REQUIRED)

add_executable(encryption-benchmark "main.cpp" "./EncryptionLibrary.cpp" "./ELgcrypt.cpp" "./ELnettle.cpp")

target_link_libraries(encryption-benchmark gcrypt)
target_link_libraries(encryption-benchmark nettle)
