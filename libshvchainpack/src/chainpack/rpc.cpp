#include <shv/chainpack/rpc.h>

namespace shv::chainpack {

const char *Rpc::protocolTypeToString(Rpc::ProtocolType pv)
{
	switch(pv) {
	case ProtocolType::ChainPack: return "ChainPack";
	case ProtocolType::Cpon: return "Cpon";
	case ProtocolType::JsonRpc: return "JsonRpc";
	case ProtocolType::Invalid: return "Invalid";
	}
	return "???";
}

} // namespace shv
