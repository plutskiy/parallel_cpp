cmake_minimum_required(VERSION 3.10.0)
project(thread_pool VERSION 0.1.0 LANGUAGES C CXX)

add_executable(thread_pool thread_pool.cpp main.cpp cyclic_barier.cpp)

find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL Threads::Threads)