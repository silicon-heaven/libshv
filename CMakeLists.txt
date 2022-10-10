cmake_minimum_required(VERSION 3.18.4)

set(WITH_SAMPLES OFF CACHE BOOL "Enable build of GUI samples")

project(libshv LANGUAGES C CXX)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves
set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")
endif()

if (NOT TARGET libnecrolog)
	add_subdirectory(3rdparty/necrolog)
endif()

add_subdirectory(libshvchainpack/c)
add_subdirectory(libshvchainpack)
add_subdirectory(libshvcore)

if(USE_QT6)
	message(STATUS "Enabling Qt6 support")
	find_package(Qt6 QUIET COMPONENTS Core Network Sql SerialPort OPTIONAL_COMPONENTS WebSockets Widgets)
	set(QtWebSockets_FOUND ${Qt6WebSockets_FOUND})
	set(Qt_FOUND ${Qt6_FOUND})
	set(QtWidgets_FOUND ${Qt6Widgets_FOUND})
else()
	find_package(Qt5 QUIET COMPONENTS Core Network Sql SerialPort OPTIONAL_COMPONENTS WebSockets Widgets)
	set(QtWebSockets_FOUND ${Qt5WebSockets_FOUND})
	set(Qt_FOUND ${Qt5_FOUND})
	set(QtWidgets_FOUND ${Qt5Widgets_FOUND})

endif()

# We'll error out if the user has explicitly enabled websocket support, but Qt WebSockets aren't available.
if(WITH_SHV_WEBSOCKETS AND NOT QtWebSockets_FOUND)
	message(FATAL_ERROR "SHV websocket support explicitly enabled, but Qt WebSockets weren't found!")
endif()

# Otherwise enable websocket support based on whether we found Qt6 WebSockets.
if (NOT DEFINED WITH_SHV_WEBSOCKETS)
	set(WITH_SHV_WEBSOCKETS ${QtWebSockets_FOUND})
endif()

if (WITH_SHV_WEBSOCKETS)
	message(STATUS "SHV websocket support enabled")
else()
	message(STATUS "SHV websocket support disabled")
endif()

if(Qt_FOUND)
	set(CMAKE_AUTOMOC ON)

	add_subdirectory(libshvcoreqt)
	add_subdirectory(libshviotqt)
	add_subdirectory(libshvbroker)

	if(QtWidgets_FOUND)
		set(CMAKE_AUTORCC ON)
		set(CMAKE_AUTOUIC ON)
		add_subdirectory(libshvvisu)
		if(WITH_SAMPLES)
			message(STATUS "samplegraph application will be built")
			add_subdirectory(samples/gui/samplegraph)
		endif()
	else()
		message(STATUS "Qt Widgets not found, libshvvisu won't be built")
	endif()

else()
	message(STATUS "Qt not found, Qt-based libs won't be built")
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
