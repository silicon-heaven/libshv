add_library(libshvchainpack-cpp SHARED
    src/chainpack/abstractstreamreader.cpp
    src/chainpack/abstractstreamwriter.cpp
    src/chainpack/accessgrant.cpp
    src/chainpack/chainpack.cpp
    src/chainpack/chainpackreader.cpp
    src/chainpack/chainpackreader1.cpp
    src/chainpack/chainpackwriter.cpp
    src/chainpack/cpon.cpp
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
    )

if (NOT WIN32)
    target_sources(libshvchainpack-cpp PRIVATE src/chainpack/socketrpcdriver.cpp)
endif()

target_link_libraries(libshvchainpack-cpp libnecrolog libshvchainpack-c)
target_include_directories(libshvchainpack-cpp PUBLIC include)
target_compile_definitions(libshvchainpack-cpp PRIVATE SHVCHAINPACK_BUILD_DLL)

function(add_shv_test test_name)
    add_executable(test_${test_name}
        tests/${test_name}.cpp
        )
    target_link_libraries(test_${test_name} libshvchainpack-cpp)
    add_test(NAME test_${test_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${test_name})
endfunction(add_shv_test)

if(BUILD_TESTING)
    add_shv_test(rpcvalue)
endif()

install(TARGETS libshvchainpack-cpp)
