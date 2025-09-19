cmake_minimum_required(VERSION 3.10.0)
project(http_client VERSION 0.1.0 LANGUAGES C CXX)

add_executable(http_client http_client.cpp main.cpp)

find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)
find_package(LibXml2 REQUIRED)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL Threads::Threads LibXml2::LibXml2)