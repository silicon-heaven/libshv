qt_add_library(libshvcoreqt
	src/data/valuechange.cpp
	src/exception.cpp
	src/log.cpp
	src/utils.cpp
	src/rpc.cpp
	src/utils/clioptions.cpp
	src/utils/versioninfo.cpp

	include/shv/coreqt/shvcoreqtglobal.h
	include/shv/coreqt/utils/versioninfo.h
	include/shv/coreqt/utils/clioptions.h
	include/shv/coreqt/rpc.h
	include/shv/coreqt/log.h
	include/shv/coreqt/data/valuechange.h
	include/shv/coreqt/utils.h
	include/shv/coreqt/exception.h
	)
target_link_libraries(libshvcoreqt PUBLIC Qt::Core libshvcore)
target_include_directories(libshvcoreqt PUBLIC include)
target_compile_definitions(libshvcoreqt PRIVATE SHVCOREQT_BUILD_DLL)

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

install(TARGETS libshvcoreqt)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/shv" TYPE INCLUDE)
