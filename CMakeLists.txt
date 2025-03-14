cmake_minimum_required (VERSION 3.5)

project(tftp_client)

add_executable(tftp_client tftp_client/main.cpp)

find_package(tftp 1.2.0)

if (NOT tftp_FOUND)
	include(FetchContent)

	FetchContent_Declare(
	  tftp
	  URL https://github.com/eoan-ermine/tftp/archive/refs/tags/v1.2.0.zip
	)

	FetchContent_GetProperties(tftp)
	if (NOT tftp_POPULATED)
		FetchContent_Populate(tftp)
	endif()

	add_library(tftp INTERFACE)
	target_include_directories(tftp INTERFACE ${tftp_SOURCE_DIR})
	target_link_libraries(tftp_client tftp)
endif()


find_package(Boost 1.74 COMPONENTS program_options system REQUIRED)

target_link_libraries(tftp_client Boost::program_options Boost::system)
