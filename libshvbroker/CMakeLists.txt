libshv_add_library(libshvbroker
	QT
	NAMESPACE broker
	SOURCES
		src/aclmanager.cpp
		src/aclmanagersqlite.cpp
		src/appclioptions.cpp
		src/appnode.cpp
		src/brokeraclnode.cpp
		src/brokerapp.cpp
		src/brokerappnode.cpp
		src/brokerrootnode.cpp
		src/clientconnectionnode.cpp
		src/clientshvnode.cpp
		src/currentclientshvnode.cpp
		src/dotbrokernode.cpp
		src/dotbrokernode.h
		src/rpc/brokertcpserver.cpp
		src/rpc/clientconnectiononbroker.cpp
		src/rpc/commonrpcclienthandle.cpp
		src/rpc/masterbrokerconnection.cpp
		src/rpc/serverconnection.cpp
		src/rpc/serverconnection.h
		src/rpc/ssl_common.cpp
		src/rpc/tcpserver.cpp
		src/rpc/tcpserver.h
		src/subscriptionsnode.cpp
		src/tunnelsecretlist.cpp
	HEADERS
		include/shv/broker/tunnelsecretlist.h
		include/shv/broker/currentclientshvnode.h
		include/shv/broker/aclmanager.h
		include/shv/broker/azureconfig.h
		include/shv/broker/ldap/ldapconfig.h
		include/shv/broker/ldap/ldap.h
		include/shv/broker/brokerapp.h
		include/shv/broker/appclioptions.h
		include/shv/broker/clientconnectionnode.h
		include/shv/broker/groupmapping.h
	)

if(LIBSHV_WITH_WEBSOCKETS)
	target_sources(libshvbroker
		PRIVATE
			src/rpc/websocketserver.cpp
			src/rpc/websocketserver.h
		)
endif()
if(OpenLDAP_FOUND)
	message(STATUS "OpenLDAP found, enabling broker support for LDAP")
	target_sources(libshvbroker PRIVATE
		src/ldap.cpp
		src/openldap_dynamic.cpp
		)
	target_include_directories(libshvbroker PUBLIC $<BUILD_INTERFACE:${OpenLDAP_INCLUDE_DIRS}>)
	target_compile_definitions(libshvbroker PUBLIC WITH_SHV_LDAP)
endif()

target_link_libraries(libshvbroker PUBLIC Qt::Sql libshviotqt)

if(BUILD_TESTING)
	function(add_shvbroker_test test_name)
		add_executable(test_broker_${test_name}
			tests/test_${test_name}.cpp
			)
		target_link_libraries(test_broker_${test_name} libshvbroker doctest::doctest)
		add_test(NAME test_broker_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_broker_${test_name}>)
	endfunction()
	add_shvbroker_test(aclmanager)
endif()
