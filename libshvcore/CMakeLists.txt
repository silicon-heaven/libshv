add_library(libshvcore
	src/exception.cpp
	src/log.cpp
	src/string.cpp
	src/stringview.cpp
	src/utils.cpp
	src/utils/abstractshvjournal.cpp
	src/utils/clioptions.cpp
	src/utils/crypt.cpp
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
	src/utils/shvurl.cpp
	src/utils/versioninfo.cpp
	)

target_precompile_headers(libshvcore
	PRIVATE
	src/utils/shvjournalcommon.h
	include/shv/core/utils/shvpath.h
	include/shv/core/utils/shvtypeinfo.h
	include/shv/core/utils/shvlogfilereader.h
	include/shv/core/utils/patternmatcher.h
	include/shv/core/utils/shvjournalfilereader.h
	include/shv/core/utils/crypt.h
	include/shv/core/utils/shvalarm.h
	include/shv/core/utils/shvurl.h
	include/shv/core/utils/shvlogrpcvaluereader.h
	include/shv/core/utils/shvlogheader.h
	include/shv/core/utils/versioninfo.h
	include/shv/core/utils/shvlogtypeinfo.h
	include/shv/core/utils/shvfilejournal.h
	include/shv/core/utils/shvjournalentry.h
	include/shv/core/utils/shvjournalfilewriter.h
	include/shv/core/utils/abstractshvjournal.h
	include/shv/core/utils/shvgetlogparams.h
	include/shv/core/utils/clioptions.h
	include/shv/core/utils/shvmemoryjournal.h
	include/shv/core/utils/shvlogfilter.h
	include/shv/core/string.h
	include/shv/core/stringview.h
	include/shv/core/assert.h
	include/shv/core/shvcoreglobal.h
	include/shv/core/log.h
	include/shv/core/utils.h
	include/shv/core/exception.h
	)

target_link_libraries(libshvcore libshvchainpack-cpp)
target_include_directories(libshvcore PUBLIC include)
target_compile_definitions(libshvcore PRIVATE SHVCORE_BUILD_DLL)

function(add_shvcore_test test_name)
	add_executable(test_core_${test_name}
		tests/test_${test_name}.cpp
		)
	target_compile_definitions(test_core_${test_name} PRIVATE DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
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
	if(NOT WIN32) # We do not support Windows paths for now.
		add_shvcore_test(clioptions)
	endif()
endif()

install(TARGETS libshvcore)
