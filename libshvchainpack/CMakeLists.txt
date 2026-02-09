libshv_add_library(libshvchainpack-cpp
	NAMESPACE chainpack
	SOURCES
		src/abstractstreamreader.cpp
		src/abstractstreamwriter.cpp
		src/accessgrant.cpp
		src/accesslevel.cpp
		src/chainpack.cpp
		src/chainpackreader.cpp
		src/chainpackwriter.cpp
		src/cponreader.cpp
		src/cponwriter.cpp
		src/datachange.cpp
		src/exception.cpp
		src/irpcconnection.cpp
		src/metamethod.cpp
		src/metatypes.cpp
		src/rpc.cpp
		src/rpcdriver.cpp
		src/rpcmessage.cpp
		src/rpcvalue.cpp
		src/tunnelctl.cpp
		src/utils.cpp
	HEADERS
		include/shv/chainpack/abstractstreamreader.h
		include/shv/chainpack/abstractstreamwriter.h
		include/shv/chainpack/accessgrant.h
		include/shv/chainpack/accesslevel.h
		include/shv/chainpack/chainpack.h
		include/shv/chainpack/chainpackreader.h
		include/shv/chainpack/chainpackwriter.h
		include/shv/chainpack/cponreader.h
		include/shv/chainpack/cponwriter.h
		include/shv/chainpack/crc32.h
		include/shv/chainpack/datachange.h
		include/shv/chainpack/exception.h
		include/shv/chainpack/irpcconnection.h
		include/shv/chainpack/metamethod.h
		include/shv/chainpack/metatypes.h
		include/shv/chainpack/rpc.h
		include/shv/chainpack/rpcdriver.h
		include/shv/chainpack/rpcmessage.h
		include/shv/chainpack/rpcvalue.h
		include/shv/chainpack/tunnelctl.h
		include/shv/chainpack/utils.h
	)

if(LIBSHV_PRECOMPILED_HEADERS)
	target_precompile_headers(libshvchainpack-cpp
		PRIVATE
		include/shv/chainpack/datachange.h
		include/shv/chainpack/rpcvalue.h
		include/shv/chainpack/utils.h
	)
endif()

if (NOT WIN32)
	target_sources(libshvchainpack-cpp
		PRIVATE
			src/socketrpcdriver.cpp
		PUBLIC
			FILE_SET HEADERS
				FILES
					include/shv/chainpack/socketrpcdriver.h
		)
endif()

target_link_libraries(libshvchainpack-cpp libnecrolog::libnecrolog libshvchainpack-c)

function(add_shv_test test_name)
	add_executable(test_${test_name}
		tests/test_${test_name}.cpp
		)
	target_link_libraries(test_${test_name} libshvchainpack-cpp doctest::doctest)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_${test_name}>)
endfunction()

function(add_shv_test_zlib test_name)
	add_executable(test_${test_name}
		tests/test_${test_name}.cpp
		)
	target_link_libraries(test_${test_name} libshvchainpack-cpp z)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_${test_name}>)
endfunction()

if(BUILD_TESTING)
	add_shv_test(chainpack)
	add_shv_test(cpon)
	add_shv_test(rpcvalue)
	add_shv_test(rpcmessage)
	add_shv_test(accessgrant)
	if (UNIX)
		add_shv_test_zlib(crc32)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		function(add_fuzz_test test_name)
			add_executable(fuzz_${test_name}
				fuzz/${test_name}.cpp
				)
			target_link_libraries(fuzz_${test_name} libshvchainpack-cpp)
			target_compile_options(fuzz_${test_name} PRIVATE "-fsanitize=fuzzer")
			target_link_options(fuzz_${test_name} PRIVATE "-fsanitize=fuzzer")
		endfunction()

		add_fuzz_test(fromCpon)
		add_fuzz_test(fromChainpack)
	endif()
endif()
