cmake_minimum_required(VERSION 3.18.4)

set(LIBSHV_USE_QT6 ON CACHE BOOL "Qt6 build")
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries instead of static")

set(LIBSHV_WITH_CHAINPACK ON CACHE BOOL "Build libshvchainpack (C)")
set(LIBSHV_WITH_CHAINPACK_CPP ON CACHE BOOL "Build libshvchainpack (C++)")
set(LIBSHV_WITH_CORE ON CACHE BOOL "Build libshvcore")
set(LIBSHV_WITH_COREQT ON CACHE BOOL "Build libshvcoreqt")
set(LIBSHV_WITH_IOTQT ON CACHE BOOL "Build libshviotqt")
set(LIBSHV_WITH_VISU ON CACHE BOOL "Build libshvvisu")
set(LIBSHV_WITH_BROKER ON CACHE BOOL "Build libshvbroker")

set(LIBSHV_WITH_ALL OFF CACHE BOOL "Build everything")
set(LIBSHV_WITH_CLI_EXAMPLES "${LIBSHV_WITH_ALL}" CACHE BOOL "Enable build of CLI examples")
set(LIBSHV_WITH_GUI_EXAMPLES "${LIBSHV_WITH_ALL}" CACHE BOOL "Enable build of GUI examples")
set(LIBSHV_WITH_LDAP "${LIBSHV_WITH_ALL}" CACHE BOOL "Enable authentization via LDAP")

set(LIBSHV_WITH_SANITIZERS OFF CACHE BOOL "Enable ASan/UBsan")
if(LIBSHV_WITH_SANITIZERS)
	set(CMAKE_C_FLAGS "-fsanitize=address,undefined ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_FLAGS "-fsanitize=address,undefined ${CMAKE_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address,undefined ${CMAKE_CXX_FLAGS}")
endif()

project(libshv LANGUAGES C CXX)
include(CTest)
include(GenerateExportHeader)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves
set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(NOT LIBSHV_USE_QT6)
	if(DEFINED ENV{LIBSHV_RUN_LUPDATE})
		message(WARNING "LIBSHV_RUN_LUPDATE env variable is set! Translation files will be refreshed.")
	endif()
endif()

set(LIBSHV_PROJECT_TRANSLATIONS
	cs_CZ
	fi_FI
	hu_HU
	it_IT
	nl_BE
	nl_NL
	pl_PL
	sr_RS
	de_DE
	sk_SK
)

function(libshv_add_library target_name)
	set(options QT)
	set(oneValueArgs NAMESPACE)
	set(multiValueArgs HEADERS SOURCES)
	cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")
	if(arg_QT)
		qt_add_library("${target_name}")
	else()
		add_library("${target_name}")
	endif()
	generate_export_header("${target_name}" EXPORT_FILE_NAME "shv/${arg_NAMESPACE}/shv${arg_NAMESPACE}_export.h")
	target_sources("${target_name}"
		PRIVATE
			${arg_SOURCES}
		PUBLIC
			FILE_SET HEADERS
				BASE_DIRS include
				FILES
					${arg_HEADERS}
				FILE_SET generated_headers
					TYPE HEADERS
					BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
					FILES "${CMAKE_CURRENT_BINARY_DIR}/shv/${arg_NAMESPACE}/shv${arg_NAMESPACE}_export.h"
	)

	add_library("libshv::${target_name}" ALIAS "${target_name}")

	install(TARGETS "${target_name}" EXPORT libshvConfig FILE_SET HEADERS FILE_SET generated_headers)
	export(TARGETS "${target_name}" NAMESPACE libshv:: FILE "${CMAKE_CURRENT_BINARY_DIR}/libshvConfig.cmake")
endfunction()

function(libshv_set_translations target langs)
	foreach(lang ${langs})
		set(TS_FILES ${TS_FILES} "${target}_${lang}.ts")
	endforeach()

	if(LIBSHV_USE_QT6)
        if(NOT "${PROJECT_TRANSLATIONS_DIR}" STREQUAL "")
            set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${PROJECT_TRANSLATIONS_DIR})
        endif()

		qt_add_lupdate("${target}" TS_FILES ${TS_FILES} OPTIONS  "-locations" "none")
		qt_add_lrelease("${target}" TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)

		foreach(ABS_PATH IN LISTS QM_FILES)
			cmake_path(GET ABS_PATH FILENAME REL_PATH)
			set_source_files_properties("${ABS_PATH}" PROPERTIES QT_RESOURCE_ALIAS "${REL_PATH}")
		endforeach()
	else()
		if(DEFINED ENV{LIBSHV_RUN_LUPDATE})
			qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
		else()
			qt5_add_translation(QM_FILES ${TS_FILES})
		endif()
		target_sources(${target} PRIVATE ${QM_FILES})
	endif()

	install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/translations)
endfunction()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(LIBSHV_WARNING_FLAGS "-Wall -Wextra -pedantic -Wcast-align -Wunused -Wpedantic -Wmisleading-indentation -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough")

	# We switched to qsizetype in Qt6, so let's get rid of this warning in Qt5 for now.
	# TODO: Remove this after dropping support for Qt5
	if(LIBSHV_USE_QT6)
		set(LIBSHV_WARNING_FLAGS "${LIBSHV_WARNING_FLAGS} -Wconversion")
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(LIBSHV_WARNING_FLAGS_CXX "-Wuseless-cast ${LIBSHV_WARNING_FLAGS_CXX}")
		endif()
	endif()

	set(LIBSHV_WARNING_FLAGS "${LIBSHV_WARNING_FLAGS} -Wno-sign-conversion")
	set(LIBSHV_WARNING_FLAGS_CXX "-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(LIBSHV_WARNING_FLAGS "-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wshadow=local ${LIBSHV_WARNING_FLAGS}")
	else() # Clang
		set(LIBSHV_WARNING_FLAGS "-Wshadow ${LIBSHV_WARNING_FLAGS}")
	endif()

	set(LIBSHV_WARNING_FLAGS_CXX "${LIBSHV_WARNING_FLAGS} ${LIBSHV_WARNING_FLAGS_CXX}")

	set(CMAKE_C_FLAGS "${LIBSHV_WARNING_FLAGS} ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${LIBSHV_WARNING_FLAGS_CXX} ${CMAKE_CXX_FLAGS}")
endif()

if(BUILD_TESTING)
	find_package(doctest QUIET)
	if(NOT doctest_FOUND)
		message(STATUS "doctest library NOT found, disabling tests")
		set(BUILD_TESTING OFF)
	endif()

	find_package(trompeloeil QUIET)
	if(NOT trompeloeil_FOUND)
		message(STATUS "trompeloeil library NOT found, disabling tests")
		set(BUILD_TESTING OFF)
	endif()
endif()

if(NOT TARGET libnecrolog)
	if(LIBSHV_USE_LOCAL_NECROLOG)
		find_package(necrolog REQUIRED)
	else()
		add_subdirectory(3rdparty/necrolog SYSTEM)
	endif()
endif()

function(check_shv_dep dependent dependent_var dependency dependency_var)
	if(${${dependent_var}})
		if(NOT ${${dependency_var}})
			message(FATAL_ERROR "${dependent} needs ${dependency}, but ${dependency} has been disabled explicitly via ${dependency_var}. Set ${dependency_var} to ON to build ${dependent}.")
		else()
			list(APPEND BUILT_COMPONENTS ${dependent})
			set(BUILT_COMPONENTS "${BUILT_COMPONENTS}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

check_shv_dep(libshvchainpack-c LIBSHV_WITH_CHAINPACK "" ON)
check_shv_dep(libshvchainpack-cpp LIBSHV_WITH_CHAINPACK_CPP libshvchainpack-c WITH_CHAINPACK)
check_shv_dep(libshvcore LIBSHV_WITH_CORE libshvchainpack-cpp WITH_CHAINPACK_CPP)
check_shv_dep(libshvcoreqt LIBSHV_WITH_COREQT libshvcore WITH_CORE)
check_shv_dep(libshviotqt LIBSHV_WITH_IOTQT libshvcoreqt WITH_COREQT)
check_shv_dep(libshvvisu LIBSHV_WITH_VISU libshviotqt WITH_IOTQT)
check_shv_dep(libshvbroker LIBSHV_WITH_BROKER libshviotqt WITH_IOTQT)
check_shv_dep(cli-examples LIBSHV_WITH_CLI_EXAMPLES libshvbroker WITH_BROKER)
check_shv_dep(gui-examples LIBSHV_WITH_GUI_EXAMPLES libshvvisu WITH_VISU)

if(LIBSHV_WITH_CHAINPACK)
	add_subdirectory(libshvchainpack/c)
endif()
if(LIBSHV_WITH_CHAINPACK_CPP)
	add_subdirectory(libshvchainpack)
endif()
if(LIBSHV_WITH_CORE)
	add_subdirectory(libshvcore)
endif()

if(LIBSHV_WITH_COREQT)
	if(LIBSHV_USE_QT6)
		message(STATUS "Enabling Qt6 support")
		if(EMSCRIPTEN)
			find_package(Qt6 COMPONENTS Core Gui LinguistTools Network OPTIONAL_COMPONENTS Svg WebSockets Widgets)
		else()
			find_package(Qt6 COMPONENTS Core Gui LinguistTools Network Sql SerialPort OPTIONAL_COMPONENTS NetworkAuth Svg WebSockets Widgets)
		endif()
		qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES ${LIBSHV_PROJECT_TRANSLATIONS})
		set(QtNetworkAuth_FOUND ${Qt6NetworkAuth_FOUND})
		set(QtWebSockets_FOUND ${Qt6WebSockets_FOUND})
		set(QtSvg_FOUND ${Qt6Svg_FOUND})
		set(Qt_FOUND ${Qt6_FOUND})
		set(QtWidgets_FOUND ${Qt6Widgets_FOUND})
	else()
		message(STATUS "Enabling Qt5 support")
		find_package(Qt5 COMPONENTS Core LinguistTools Network Sql SerialPort OPTIONAL_COMPONENTS Svg WebSockets Widgets)
		set(QtWebSockets_FOUND ${Qt5WebSockets_FOUND})
		set(QtSvg_FOUND ${Qt6Svg_FOUND})
		set(Qt_FOUND ${Qt5_FOUND})
		set(QtWidgets_FOUND ${Qt5Widgets_FOUND})

		function(qt_add_library target)
			add_library(${target} ${ARGN})
			if(WIN32)
				target_compile_definitions(${target} PRIVATE UNICODE)
				target_compile_definitions(${target} PRIVATE _UNICODE)
			endif()
		endfunction()

		function(qt_add_executable)
			add_executable(${ARGV})
		endfunction()
	endif()

	if(QT_KNOWN_POLICY_QTP0003)
		qt_policy(SET QTP0003 NEW)
	endif()
endif()

# We'll error out if the user has explicitly enabled websocket support, but Qt WebSockets aren't available.
if(LIBSHV_WITH_WEBSOCKETS AND NOT QtWebSockets_FOUND)
	message(FATAL_ERROR "SHV websocket support explicitly enabled, but Qt WebSockets weren't found!")
endif()

# Otherwise enable websocket support based on whether we found Qt6 WebSockets.
if(NOT DEFINED LIBSHV_WITH_WEBSOCKETS)
	set(LIBSHV_WITH_WEBSOCKETS ${QtWebSockets_FOUND})
endif()

# Otherwise enable OAuth2 Azure support based on whether we found Qt6 NetworkAuth.
if(NOT DEFINED LIBSHV_WITH_OAUTH2_AZURE)
	set(LIBSHV_WITH_OAUTH2_AZURE ${QtNetworkAuth_FOUND})
endif()

if(LIBSHV_WITH_OAUTH2_AZURE)
	message(STATUS "OAuth2 Azure support enabled")
else()
	message(STATUS "OAuth2 Azure support disabled")
endif()

if(LIBSHV_WITH_WEBSOCKETS)
	message(STATUS "SHV websocket support enabled")
else()
	message(STATUS "SHV websocket support disabled")
endif()

set(LIBSHV_WITH_SVG ${QtSvg_FOUND})
if(LIBSHV_WITH_SVG)
	message(STATUS "SVG support enabled")
else()
	message(STATUS "SVG support disabled")
endif()

if(LIBSHV_WITH_CORE)
	if(Qt_FOUND)
		# We'll error out if the user has explicitly enabled Azure OAuth2 support, but Qt NetworkAuth isn't available.
		if(LIBSHV_WITH_OAUTH2_AZURE AND NOT QtNetworkAuth_FOUND)
			message(FATAL_ERROR "OAuth2 Azure explicitly enabled, but Qt NetworkAuth wasn't found!")
		endif()

		set(CMAKE_AUTOMOC ON)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(CMAKE_AUTOMOC_MOC_OPTIONS "-b${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt_moc_disable_warnings.h")
		endif()

		if(LIBSHV_WITH_COREQT)
			add_subdirectory(libshvcoreqt)
		endif()
		if(LIBSHV_WITH_IOTQT)
			add_subdirectory(libshviotqt)
		endif()
		if(NOT EMSCRIPTEN)
			set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
			if(LIBSHV_WITH_LDAP)
				find_package(PkgConfig REQUIRED)
				pkg_check_modules(OpenLDAP REQUIRED ldap>=2.5.0)
			endif()
			if(LIBSHV_WITH_BROKER)
				add_subdirectory(libshvbroker)
			endif()
		endif()

		if(LIBSHV_WITH_CLI_EXAMPLES AND NOT EMSCRIPTEN)
			message(STATUS "CLI examples will be built")
			add_subdirectory(examples/cli/minimalshvclient)
			add_subdirectory(examples/cli/shvdevice)
			add_subdirectory(examples/cli/minimalshvbroker)
			add_subdirectory(examples/cli/shvcall)
		endif()
		if(QtWidgets_FOUND)
			set(CMAKE_AUTORCC ON)
			set(CMAKE_AUTOUIC ON)
			if(LIBSHV_WITH_VISU)
				add_subdirectory(libshvvisu)
			endif()
			if(LIBSHV_WITH_GUI_EXAMPLES AND NOT EMSCRIPTEN)
				message(STATUS "GUI example samplegraph application will be built")
				add_subdirectory(examples/gui/samplegraph)
			endif()
		else()
			message(STATUS "Qt Widgets not found, libshvvisu won't be built")
		endif()
	else()
		message(STATUS "Qt not found, Qt-based libs won't be built")
	endif()
endif()

option(BUILD_SHV_UTILS "Whether to build ccp2cp, cp2cp, and cpmerge" ON)
check_shv_dep(shv-utils BUILD_SHV_UTILS libshvcoreqt LIBSHV_WITH_CORE)

# Utils
if(BUILD_SHV_UTILS)
	message(STATUS "SHV utils will be built")
	# ccp2cp
	add_executable(ccp2cp utils/ccp2cp/main.c)
	target_link_libraries(ccp2cp libshvchainpack-c)

	# cp2cp
	add_executable(cp2cp utils/cp2cp/main.cpp)
	target_link_libraries(cp2cp libshvchainpack-cpp)

	# cp2log
	add_executable(cp2log utils/cp2log/main.cpp)
	target_link_libraries(cp2log libshvcore)

	# cpmerge
	add_executable(cpmerge utils/cpmerge/main.cpp)
	target_link_libraries(cpmerge libshvchainpack-cpp)

	install(TARGETS ccp2cp cp2cp cpmerge)
else()
	message(STATUS "SHV utils WON'T be built")
endif()

if(NOT DEFINED BUILT_COMPONENTS)
	message(FATAL_ERROR "All components have been disabled, nothing to build.")
endif()

if(NOT EMSCRIPTEN)
	install(EXPORT libshvConfig DESTINATION share/libshv/cmake NAMESPACE libshv::)
endif()
list(JOIN BUILT_COMPONENTS ", " BUILT_COMPONENTS_STR)

message(STATUS "Enabled SHV components: ${BUILT_COMPONENTS_STR}")
