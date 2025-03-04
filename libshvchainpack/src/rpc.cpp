#include <shv/chainpack/rpc.h>

namespace shv::chainpack {
// NOLINTBEGIN(readability-redundant-declaration) - needed to fix debug build on MinGW
constexpr const char* Rpc::RCV_LOG_ARROW;
constexpr const char* Rpc::PAR_SIGNAL;
// NOLINTEND(readability-redundant-declaration)

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
