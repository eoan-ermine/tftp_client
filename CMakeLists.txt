cmake_minimum_required (VERSION 3.5)

project(tftp_client)

find_package(tftp_common 1.0.0 REQUIRED)
find_package(Boost 1.74 COMPONENTS program_options system REQUIRED)

add_executable(tftp_client tftp_client/main.cpp)
target_link_libraries(tftp_client Boost::program_options Boost::system)
