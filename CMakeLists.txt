cmake_minimum_required(VERSION 3.18.4)

set(WITH_SAMPLES OFF CACHE BOOL "Enable build of GUI samples")

project(libshv LANGUAGES C CXX)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")
endif()

if (NOT TARGET libnecrolog)
	add_subdirectory(3rdparty/necrolog)
endif()

add_subdirectory(libshvchainpack/c)
add_subdirectory(libshvchainpack)
add_subdirectory(libshvcore)

find_package(Qt5 QUIET COMPONENTS Core Network Sql SerialPort OPTIONAL_COMPONENTS WebSockets Widgets)

# We'll error out if the user has explicitly enabled websocket support, but Qt5 WebSockets aren't available.
if(WITH_SHV_WEBSOCKETS AND NOT Qt5WebSockets_FOUND)
	message(FATAL_ERROR "SHV websocket support explicitly enabled, but Qt5 WebSockets weren't found!")
endif()

# Otherwise enable websocket support based on whether we found Qt5 WebSockets.
if (NOT DEFINED WITH_SHV_WEBSOCKETS)
	set(WITH_SHV_WEBSOCKETS ${Qt5WebSockets_FOUND})
endif()

if (WITH_SHV_WEBSOCKETS)
	message(STATUS "SHV websocket support enabled")
else()
	message(STATUS "SHV websocket support disabled")
endif()

if(Qt5_FOUND)
	set(CMAKE_AUTOMOC ON)

	add_subdirectory(libshvcoreqt)
	add_subdirectory(libshviotqt)
	add_subdirectory(libshvbroker)

	if(Qt5Widgets_FOUND)
		set(CMAKE_AUTORCC ON)
		set(CMAKE_AUTOUIC ON)
		add_subdirectory(libshvvisu)
		if(WITH_SAMPLES)
			message(STATUS "samplegraph application will be built")
			add_subdirectory(samples/gui/samplegraph)
		endif()
	else()
		message(STATUS "Qt5 Widgets not found, libshvvisu won't be built")
	endif()

else()
	message(STATUS "Qt5 not found, Qt-based libs won't be built")
endif()

# Utils
# ccp2cp
add_executable(ccp2cp utils/ccp2cp/main.c)
target_link_libraries(ccp2cp libshvchainpack-c)

# cp2cp
add_executable(cp2cp utils/cp2cp/main.cpp)
target_link_libraries(cp2cp libshvchainpack-cpp)

# cpmerge
add_executable(cpmerge utils/cpmerge/main.cpp)
target_link_libraries(cpmerge libshvchainpack-cpp)

install(TARGETS ccp2cp cp2cp cpmerge)
