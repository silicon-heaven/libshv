add_library(libshvchainpack-cpp
	src/chainpack/abstractstreamreader.cpp
	src/chainpack/abstractstreamwriter.cpp
	src/chainpack/accessgrant.cpp
	src/chainpack/chainpack.cpp
	src/chainpack/chainpackreader.cpp
	#src/chainpack/chainpackreader1.cpp
	src/chainpack/chainpackwriter.cpp
	src/chainpack/cponreader.cpp
	src/chainpack/cponwriter.cpp
	src/chainpack/datachange.cpp
	src/chainpack/exception.cpp
	src/chainpack/irpcconnection.cpp
	src/chainpack/metamethod.cpp
	src/chainpack/metatypes.cpp
	src/chainpack/rpc.cpp
	src/chainpack/rpcdriver.cpp
	src/chainpack/rpcmessage.cpp
	src/chainpack/rpcvalue.cpp
	src/chainpack/tunnelctl.cpp
	src/chainpack/utils.cpp
	include/shv/chainpack/crc32.h
	)

target_precompile_headers(libshvchainpack-cpp
	PRIVATE
	src/chainpack/cpon.h
	include/shv/chainpack/irpcconnection.h
	include/shv/chainpack/accessgrant.h
	include/shv/chainpack/metatypes.h
	include/shv/chainpack/rpcvalue.h
	include/shv/chainpack/cponreader.h
	include/shv/chainpack/socketrpcdriver.h
	include/shv/chainpack/shvchainpackglobal.h
	include/shv/chainpack/chainpackwriter.h
	include/shv/chainpack/cponwriter.h
	include/shv/chainpack/datachange.h
	include/shv/chainpack/crc32.h
	include/shv/chainpack/abstractstreamreader.h
	include/shv/chainpack/rpc.h
	include/shv/chainpack/abstractstreamwriter.h
	include/shv/chainpack/rpcmessage.h
	include/shv/chainpack/chainpackreader.h
	include/shv/chainpack/metamethod.h
	include/shv/chainpack/chainpack.h
	include/shv/chainpack/rpcdriver.h
	include/shv/chainpack/utils.h
	include/shv/chainpack/tunnelctl.h
	include/shv/chainpack/exception.h
	)

if (NOT WIN32)
	target_sources(libshvchainpack-cpp PRIVATE src/chainpack/socketrpcdriver.cpp)
endif()

target_link_libraries(libshvchainpack-cpp libnecrolog libshvchainpack-c)
target_include_directories(libshvchainpack-cpp PUBLIC include)
target_compile_definitions(libshvchainpack-cpp PRIVATE SHVCHAINPACK_BUILD_DLL)

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

install(TARGETS libshvchainpack-cpp)
