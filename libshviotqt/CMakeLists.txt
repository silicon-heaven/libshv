set(HEADER_FILES
	include/shv/iotqt/acl/aclmountdef.h
	include/shv/iotqt/acl/acluser.h
	include/shv/iotqt/acl/aclrole.h
	include/shv/iotqt/acl/aclpassword.h
	include/shv/iotqt/acl/aclroleaccessrules.h
	include/shv/iotqt/node/shvnodetree.h
	include/shv/iotqt/node/shvnode.h
	include/shv/iotqt/node/localfsnode.h
	include/shv/iotqt/node/filenode.h
	include/shv/iotqt/shviotqtglobal.h
	include/shv/iotqt/utils/network.h
	include/shv/iotqt/rpc/deviceconnection.h
	include/shv/iotqt/rpc/tcpserver.h
	include/shv/iotqt/rpc/rpcresponsecallback.h
	include/shv/iotqt/rpc/websocket.h
	include/shv/iotqt/rpc/socket.h
	include/shv/iotqt/rpc/clientconnection.h
	include/shv/iotqt/rpc/serverconnection.h
	include/shv/iotqt/rpc/serialportsocket.h
	include/shv/iotqt/rpc/clientappclioptions.h
	include/shv/iotqt/rpc/deviceappclioptions.h
	include/shv/iotqt/rpc/rpccall.h
	include/shv/iotqt/rpc/socketrpcconnection.h
	include/shv/iotqt/utils.h
	)

qt_add_library(libshviotqt
	src/acl/aclmountdef.cpp
	src/acl/aclpassword.cpp
	src/acl/aclrole.cpp
	src/acl/aclroleaccessrules.cpp
	src/acl/acluser.cpp
	src/node/filenode.cpp
	src/node/localfsnode.cpp
	src/node/shvnode.cpp
	src/node/shvnodetree.cpp
	src/rpc/clientappclioptions.cpp
	src/rpc/clientconnection.cpp
	src/rpc/deviceappclioptions.cpp
	src/rpc/deviceconnection.cpp
	src/rpc/rpcresponsecallback.cpp
	src/rpc/serverconnection.cpp
	src/rpc/socket.cpp
	src/rpc/socketrpcconnection.cpp
	src/rpc/tcpserver.cpp
	src/utils.cpp
	src/utils/network.cpp
	${HEADER_FILES}
)

target_precompile_headers(libshviotqt
	PRIVATE
	${HEADER_FILES}
	)

target_link_libraries(libshviotqt PUBLIC Qt::Network libshvcoreqt)

if(NOT EMSCRIPTEN)
	target_link_libraries(libshviotqt PUBLIC Qt::SerialPort)
	target_sources(libshviotqt PRIVATE
		src/rpc/serialportsocket.cpp
		)
endif()

target_include_directories(libshviotqt PUBLIC include)
target_compile_definitions(libshviotqt PRIVATE SHVIOTQT_BUILD_DLL)

if(WITH_SHV_WEBSOCKETS)
	target_sources(libshviotqt PRIVATE
		src/rpc/websocket.cpp
		)
	target_compile_definitions(libshviotqt PUBLIC WITH_SHV_WEBSOCKETS)
	target_link_libraries(libshviotqt PUBLIC Qt::WebSockets)
endif()

function(add_shviotqt_serialportsocket_test test_name)
	set(HEADER_FILES
		tests/serialportsocket/mockrpcconnection.h
		tests/serialportsocket/mockserialport.h
		)
	add_executable(test_iotqt_${test_name}
		tests/serialportsocket/test_${test_name}.cpp
		tests/serialportsocket/mockserialport.cpp
		tests/serialportsocket/mockrpcconnection.cpp
		${HEADER_FILES}
	)

	target_precompile_headers(test_iotqt_${test_name}
		PRIVATE
		${HEADER_FILES}
		)

	#target_compile_definitions(test_iotqt_${test_name} PRIVATE DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
	target_link_libraries(test_iotqt_${test_name} libshviotqt doctest::doctest)
	add_test(NAME test_iotqt_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_iotqt_${test_name}>)
endfunction(add_shviotqt_serialportsocket_test)

if(BUILD_TESTING)
	add_shviotqt_serialportsocket_test(serialportsocket)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_serialportsocket")
	set_property(TEST test_iotqt_serialportsocket APPEND PROPERTY ENVIRONMENT "TESTS_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_serialportsocket")
endif()

install(TARGETS libshviotqt)
