qt_add_library(libshvbroker
	src/aclmanager.cpp
	src/aclmanagersqlite.cpp
	src/appclioptions.cpp
	src/brokeraclnode.cpp
	src/brokerapp.cpp
	src/brokerappnode.cpp
	src/brokerrootnode.cpp
	src/clientconnectionnode.cpp
	src/clientshvnode.cpp
	src/currentclientshvnode.cpp
	src/rpc/brokertcpserver.cpp
	src/rpc/clientconnectiononbroker.cpp
	src/rpc/commonrpcclienthandle.cpp
	src/rpc/masterbrokerconnection.cpp
	src/rpc/ssl_common.cpp
	src/subscriptionsnode.cpp
	src/tunnelsecretlist.cpp
	)

if(WITH_SHV_WEBSOCKETS)
	target_sources(libshvbroker PRIVATE
		src/rpc/websocketserver.cpp
		)
endif()
if(OpenLDAP_FOUND)
	message(STATUS "OpenLDAP found, enabling broker support for LDAP")
	target_sources(libshvbroker PRIVATE
		src/ldap.cpp
		)
	target_link_libraries(libshvbroker PUBLIC OpenLDAP::OpenLDAP)
	target_compile_definitions(libshvbroker PRIVATE WITH_SHV_LDAP)
endif()

target_link_libraries(libshvbroker PUBLIC Qt::Sql libshviotqt)
target_include_directories(libshvbroker PUBLIC include)
target_compile_definitions(libshvbroker PRIVATE SHVBROKER_BUILD_DLL)

install(TARGETS libshvbroker)
