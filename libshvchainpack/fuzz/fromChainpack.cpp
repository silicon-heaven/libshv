#include <shv/chainpack/rpcvalue.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string err;
    try {
        auto rpc_value = shv::chainpack::RpcValue::fromChainPack({reinterpret_cast<const char*>(data), size}, &err);
    } catch(std::exception& ex) {
        return 0;
    }
    return 0;  // Values other than 0 and -1 are reserved for future use.
}
