qt_add_library(libshvbroker SHARED
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

target_link_libraries(libshvbroker PUBLIC Qt::Sql libshviotqt)
target_include_directories(libshvbroker PUBLIC include)
target_compile_definitions(libshvbroker PRIVATE SHVBROKER_BUILD_DLL)

install(TARGETS libshvbroker)
