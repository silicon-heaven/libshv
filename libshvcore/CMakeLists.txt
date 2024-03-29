add_library(libshvcore
	src/exception.cpp
	src/log.cpp
	src/string.cpp
	src/stringview.cpp
	src/utils.cpp
	src/utils/abstractshvjournal.cpp
	src/utils/clioptions.cpp
	src/utils/crypt.cpp
	src/utils/getlog.cpp
	src/utils/patternmatcher.cpp
	src/utils/shvalarm.cpp
	src/utils/shvfilejournal.cpp
	src/utils/shvgetlogparams.cpp
	src/utils/shvjournalcommon.cpp
	src/utils/shvjournalentry.cpp
	src/utils/shvjournalfilereader.cpp
	src/utils/shvjournalfilewriter.cpp
	src/utils/shvlogfilereader.cpp
	src/utils/shvlogfilter.cpp
	src/utils/shvlogheader.cpp
	src/utils/shvlogrpcvaluereader.cpp
	src/utils/shvmemoryjournal.cpp
	src/utils/shvpath.cpp
	src/utils/shvtypeinfo.cpp
	src/utils/versioninfo.cpp
	)
add_library(libshv::libshvcore ALIAS libshvcore)

target_link_libraries(libshvcore libshvchainpack-cpp)
target_include_directories(libshvcore PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	)
target_compile_definitions(libshvcore PRIVATE SHVCORE_BUILD_DLL)

function(add_shvcore_test test_name)
	add_executable(test_core_${test_name}
		tests/test_${test_name}.cpp
		)
	target_compile_definitions(test_core_${test_name} PRIVATE DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/files/test_core_${test_name}")
	target_compile_definitions(test_core_${test_name} PRIVATE TEST_FILES_DIR="${CMAKE_CURRENT_BINARY_DIR}/tests/files/test_core_${test_name}")
	target_link_libraries(test_core_${test_name} libshvcore doctest::doctest)
	add_test(NAME test_core_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_core_${test_name}>)
endfunction(add_shvcore_test)

if(BUILD_TESTING)
	add_shvcore_test(shvtypeinfo)
	add_shvcore_test(stringview)
	add_shvcore_test(shvpath)
	add_shvcore_test(crypt)
	add_shvcore_test(shvlog)
	add_shvcore_test(shvjournalfilereader)
	add_shvcore_test(utils)
	add_shvcore_test(getlog)
	if(NOT WIN32) # We do not support Windows paths for now.
		add_shvcore_test(clioptions)
	endif()
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/shv" TYPE INCLUDE)

install(TARGETS libshvcore EXPORT libshvConfig)
export(TARGETS libshvcore NAMESPACE libshv:: FILE "${CMAKE_CURRENT_BINARY_DIR}/libshvConfig.cmake")
install(EXPORT libshvConfig DESTINATION share/libshv/cmake NAMESPACE libshv::)
