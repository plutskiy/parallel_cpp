cmake_minimum_required(VERSION 3.10.0)
project(philosophers VERSION 0.1.0 LANGUAGES C CXX)

add_executable(philosophers philosophers.cpp)


find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)