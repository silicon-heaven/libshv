#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/deviceappclioptions.h>

#include <shv/core/log.h>
#include <shv/core/string.h>

#include <QCoreApplication>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv::iotqt::rpc {

DeviceConnection::DeviceConnection(QObject *parent)
	: Super(parent)
{
}

const shv::chainpack::RpcValue::Map& DeviceConnection::deviceOptions() const
{
	return connectionOptions().asMap().value(cp::Rpc::KEY_DEVICE).asMap();
}

shv::chainpack::RpcValue DeviceConnection::deviceId() const
{
	return deviceOptions().value(cp::Rpc::KEY_DEVICE_ID);
}

void DeviceConnection::setCliOptions(const DeviceAppCliOptions *cli_opts)
{
	Super::setCliOptions(cli_opts);
	if(cli_opts) {
		chainpack::RpcValue::Map opts = connectionOptions().asMap();
		cp::RpcValue::Map dev;
		std::string device_id = cli_opts->deviceId();
		std::string device_id_from_file;
		{
			std::string fn = cli_opts->deviceIdFile();
			if(!fn.empty()) {
				std::ifstream ifs(fn, std::ios::binary);
				if(ifs)
					ifs >> device_id_from_file;
				else
					shvError() << "Cannot read device ID file:" << fn;
			}
		}
		if(device_id.empty()) {
			device_id = device_id_from_file;
		}
		else {
			shv::core::string::replace(device_id, "{{deviceIdFile}}", device_id_from_file);
			shv::core::string::replace(device_id, "{{appName}}", QCoreApplication::applicationName().toStdString());
		}
		if(!device_id.empty())
			dev[cp::Rpc::KEY_DEVICE_ID] = device_id;
		if(!cli_opts->mountPoint().empty())
			dev[cp::Rpc::KEY_MOUT_POINT] = cli_opts->mountPoint();
		opts[cp::Rpc::KEY_DEVICE] = dev;
		if(device_id.empty() && cli_opts->mountPoint().empty())
			shvWarning() << "Neither deviceId nor mountPoint defined for device, deviceid file:" << cli_opts->deviceId();
		setConnectionOptions(opts);
	}
}

} // namespace shv
