#include <shv/iotqt/rpc/deviceappclioptions.h>

#include <shv/core/utils/shvfilejournal.h>

namespace cp = shv::chainpack;

namespace shv::iotqt::rpc {

DeviceAppCliOptions::DeviceAppCliOptions()
{
	addOption("device.mountPoint").setType(cp::RpcValue::Type::String).setNames({"-m", "--mount", "--mount-point"}).setComment("Shv tree, where device should be mounted to. Only paths beginning with test/ are enabled. --mount-point version is deprecated");
	addOption("device.id").setType(cp::RpcValue::Type::String).setNames("--id", "--device-id").setComment("Device ID");
	addOption("device.idFile").setType(cp::RpcValue::Type::String).setNames("--idf", "--device-id-file").setComment("Device ID file");
	addOption("shvJournal.dir").setType(cp::RpcValue::Type::String).setNames("--jd", "--shvjournal-dir").setComment("SHV journal directory, /tmp/shvjournal/[app_name] if not specified.");
	addOption("shvJournal.fileSizeLimit").setType(cp::RpcValue::Type::String).setNames("--jfs", "--shvjournal-file-size-limit")
			.setComment("Maximum SHV journal file size, multiplicator postfix is possible, like 4K, 1M or 2G")
			.setDefaultValue(std::to_string(core::utils::ShvFileJournal::DEFAULT_FILE_SIZE_LIMIT));
	addOption("shvJournal.sizeLimit").setType(cp::RpcValue::Type::String).setNames("--js", "--shvjournal-size-limit")
			.setComment("Maximum SHV journal size, multiplicator postfix is possible, like 4K, 1M or 2G")
			.setDefaultValue(std::to_string(core::utils::ShvFileJournal::DEFAULT_JOURNAL_SIZE_LIMIT));
}

} // namespace shv
