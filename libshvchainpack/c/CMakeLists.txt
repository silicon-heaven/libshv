add_library(libshvchainpack-c STATIC
	cchainpack.c
	ccpcp.c
	ccpcp_convert.c
	ccpon.c
	)
target_include_directories(libshvchainpack-c PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/shv" TYPE INCLUDE)

install(TARGETS libshvchainpack-c EXPORT libshvConfig)
export(TARGETS libshvchainpack-c NAMESPACE libshvchainpack-c:: FILE "${CMAKE_CURRENT_BINARY_DIR}/libshvConfig.cmake")
