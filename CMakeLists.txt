cmake_minimum_required (VERSION 3.5)

project(tftp_client)

find_package(tftp_common 1.1.0)

if (NOT tftp_common_FOUND)
	include(FetchContent)

	FetchContent_Declare(
	  tftp_common
	  URL https://github.com/eoan-ermine/tftp_common/archive/refs/tags/v1.1.0.zip
	)

	FetchContent_GetProperties(tftp_common)
	if (NOT tftp_common_POPULATED)
		FetchContent_Populate(tftp_common)
	endif()

	add_library(tftp_common INTERFACE)
	target_include_directories(tftp_common INTERFACE ${tftp_common_SOURCE_DIR})
endif()

find_package(Boost 1.74 COMPONENTS program_options system REQUIRED)

add_executable(tftp_client tftp_client/main.cpp)
target_link_libraries(tftp_client tftp_common Boost::program_options Boost::system)
