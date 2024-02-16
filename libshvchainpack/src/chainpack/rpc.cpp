#include <shv/chainpack/rpc.h>

namespace shv::chainpack {

const char *Rpc::protocolTypeToString(Rpc::ProtocolType pv)
{
	switch(pv) {
	case ProtocolType::Cpon: return "Cpon";
	case ProtocolType::ChainPack: return "ChainPack";
	case ProtocolType::Invalid: return "Invalid";
	}
	return "???";
}

} // namespace shv
