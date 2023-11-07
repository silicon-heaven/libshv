#pragma once

#include "clientappclioptions.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT DeviceAppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::ClientAppCliOptions;
public:
	DeviceAppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "device.mountPoint", m, setM, ountPoint)
	CLIOPTION_GETTER_SETTER2(std::string, "device.id", d, setD, eviceId)
	CLIOPTION_GETTER_SETTER2(std::string, "device.idFile", d, setD, eviceIdFile)
	CLIOPTION_GETTER_SETTER2(std::string, "shvJournal.dir", s, setS, hvJournalDir)
	CLIOPTION_GETTER_SETTER2(std::string, "shvJournal.fileSizeLimit", s, setS, hvJournalFileSizeLimit)
	CLIOPTION_GETTER_SETTER2(std::string, "shvJournal.sizeLimit", s, setS, hvJournalSizeLimit)
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
