libshv_add_library(libshvcoreqt
	QT
	NAMESPACE coreqt
	SOURCES
		src/data/rpcsqlresult.cpp
		src/data/valuechange.cpp
		src/exception.cpp
		src/log.cpp
		src/rpc.cpp
		src/utils.cpp
		src/utils/versioninfo.cpp
	HEADERS
		include/shv/coreqt/data/rpcsqlresult.h
		include/shv/coreqt/data/valuechange.h
		include/shv/coreqt/exception.h
		include/shv/coreqt/log.h
		include/shv/coreqt/rpc.h
		include/shv/coreqt/utils.h
		include/shv/coreqt/utils/versioninfo.h
	)

if(LIBSHV_PRECOMPILED_HEADERS)
	target_precompile_headers(libshvcoreqt
		PRIVATE
		include/shv/coreqt/rpc.h
		include/shv/coreqt/utils.h
	)
endif()

target_link_libraries(libshvcoreqt PUBLIC Qt::Core libshvcore)

function(add_shvcoreqt_test test_name)
	add_executable(test_coreqt_${test_name}
		tests/test_${test_name}.cpp
		)
	target_compile_definitions(test_coreqt_${test_name} PRIVATE DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
	target_link_libraries(test_coreqt_${test_name} libshvcoreqt doctest::doctest)
	add_test(NAME test_coreqt_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_coreqt_${test_name}>)
endfunction(add_shvcoreqt_test)

if(BUILD_TESTING)
	add_shvcoreqt_test(rpc_variant)
	add_shvcoreqt_test(utils)
endif()
