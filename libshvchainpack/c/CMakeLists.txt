add_library(libshvchainpack-c STATIC)

target_sources(libshvchainpack-c
	PRIVATE
		cchainpack.c
		ccpcp.c
		ccpcp_convert.c
		ccpon.c
	PUBLIC
		FILE_SET HEADERS
			BASE_DIRS include
			FILES
				include/shv/chainpack/ccpcp.h
				include/shv/chainpack/ccpcp_convert.h
				include/shv/chainpack/compat.h
				include/shv/chainpack/cchainpack.h
				include/shv/chainpack/pack_double.h
				include/shv/chainpack/ccpon.h
	)

function(add_shv_test test_name)
	add_executable(test_${test_name}
		tests/test_${test_name}.c
		)
	target_link_libraries(test_${test_name} libshvchainpack-c)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_${test_name}>)
endfunction(add_shv_test)

if(BUILD_TESTING)
	add_shv_test(ccpcp)
endif()

install(TARGETS libshvchainpack-c EXPORT libshvConfig FILE_SET HEADERS)
export(TARGETS libshvchainpack-c NAMESPACE libshvchainpack-c:: FILE "${CMAKE_CURRENT_BINARY_DIR}/libshvConfig.cmake")
