add_library(libshviotqt SHARED
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
    src/rpc/rpc.cpp
    src/rpc/rpcresponsecallback.cpp
    src/rpc/serverconnection.cpp
    src/rpc/socket.cpp
    src/rpc/socketrpcconnection.cpp
    src/rpc/tcpserver.cpp
    src/utils.cpp
    src/utils/network.cpp
    )
target_link_libraries(libshviotqt PUBLIC Qt::Network Qt::SerialPort libshvcoreqt)
target_include_directories(libshviotqt PUBLIC include)
target_compile_definitions(libshviotqt PRIVATE SHVIOTQT_BUILD_DLL)

if(WITH_SHV_WEBSOCKETS)
    target_sources(libshviotqt PRIVATE
        src/rpc/websocket.cpp
        )
    target_compile_definitions(libshviotqt PUBLIC WITH_SHV_WEBSOCKETS)
    target_link_libraries(libshviotqt PUBLIC Qt::WebSockets)
endif()

install(TARGETS libshviotqt)
