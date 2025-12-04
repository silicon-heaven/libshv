#include "masterbrokerconnection.h"
#include <shv/broker/brokerapp.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/deviceappclioptions.h>

namespace cp = shv::chainpack;

namespace shv::broker::rpc {

MasterBrokerConnection::MasterBrokerConnection(QObject *parent)
	: Super(core::utils::makeUserAgent("shvbroker-cpp"), parent)
{

}

shv::chainpack::RpcValue MasterBrokerConnection::options()
{
	return m_options;
}

void MasterBrokerConnection::setOptions(const shv::chainpack::RpcValue &slave_broker_options)
{
	m_options = slave_broker_options;
	if(slave_broker_options.isMap()) {
		const cp::RpcValue::Map &m = slave_broker_options.asMap();

		shv::iotqt::rpc::DeviceAppCliOptions device_opts;

		const cp::RpcValue::Map &server = m.valref("server").asMap();
		device_opts.setServerHost(server.value("host", "localhost").asString());
		device_opts.setServerPeerVerify(server.value("peerVerify", true).toBool());

		const cp::RpcValue::Map &login = m.valref(cp::Rpc::KEY_LOGIN).asMap();
		static const std::vector<std::string> keys {"user", "password", "passwordFile", "type"};
		for(const std::string &key : keys) {
			if(login.hasKey(key))
				device_opts.setValue("login." + key, login.value(key).asString());
		}
		const cp::RpcValue::Map &rpc = m.valref("rpc").asMap();
		if(rpc.count("heartbeatInterval") == 1)
			device_opts.setHeartBeatInterval(rpc.value("heartbeatInterval", 60).toInt());
		if(rpc.count("reconnectInterval") == 1)
			device_opts.setReconnectInterval(rpc.value("reconnectInterval").toInt());

		const cp::RpcValue::Map &device = m.valref(cp::Rpc::KEY_DEVICE).asMap();
		if(device.count("id") == 1)
			device_opts.setDeviceId(device.value("id").asString());
		if(device.count("idFile") == 1)
			device_opts.setDeviceIdFile(device.value("idFile").asString());
		if(device.count("mountPoint") == 1)
			device_opts.setMountPoint(device.value("mountPoint").asString());
		setCliOptions(&device_opts);
		{
			chainpack::RpcValue::Map opts = connectionOptions().asMap();
			cp::RpcValue::Map broker;
			opts[cp::Rpc::KEY_BROKER] = broker;
			setConnectionOptions(opts);
		}
	}
	m_exportedShvPath = slave_broker_options.asMap().value("exportedShvPath").asString();
}

std::string MasterBrokerConnection::loggedUserName()
{
	return std::string();
}

bool MasterBrokerConnection::isConnectedAndLoggedIn() const
{
	return isSocketConnected() && !isLoginPhase();
}

bool MasterBrokerConnection::isSlaveBrokerConnection() const
{
	return false;
}

bool MasterBrokerConnection::isMasterBrokerConnection() const
{
	return true;
}

void MasterBrokerConnection::sendRpcFrame(chainpack::RpcFrame &&frame)
{
	logRpcMsg() << chainpack::Rpc::SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< RpcDriver::frameToPrettyCpon(frame);
	Super::sendRpcFrame(std::move(frame));
}

void MasterBrokerConnection::sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	Super::sendRpcMessage(rpc_msg);
}

CommonRpcClientHandle::Subscription MasterBrokerConnection::createSubscription(const std::string &shv_path, const std::string &method, const std::string& source)
{
	return Subscription(masterExportedToLocalPath(shv_path), method, source);
}

std::string MasterBrokerConnection::toSubscribedPath(const std::string &signal_path) const
{
	return localPathToMasterExported(signal_path);
}

std::string MasterBrokerConnection::masterExportedToLocalPath(const std::string &master_path) const
{
	if(m_exportedShvPath.empty())
		return master_path;
	static const std::string DIR_BROKER = cp::Rpc::DIR_BROKER;
	if(shv::core::utils::ShvPath::startsWithPath(master_path, DIR_BROKER))
		return master_path;
	return shv::core::utils::ShvPath(m_exportedShvPath).appendPath(master_path).asString();
}

std::string MasterBrokerConnection::localPathToMasterExported(const std::string &local_path) const
{
	if(m_exportedShvPath.empty())
		return local_path;
	size_t pos;
	if(shv::core::utils::ShvPath::startsWithPath(local_path, m_exportedShvPath, &pos))
		return local_path.substr(pos);
	return local_path;
}

const std::string& MasterBrokerConnection::exportedShvPath() const
{
	return m_exportedShvPath;
}

void MasterBrokerConnection::onRpcFrameReceived(chainpack::RpcFrame &&frame)
{
	logRpcMsg() << chainpack::Rpc::RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< RpcDriver::frameToPrettyCpon(frame);
	try {
		if(isLoginPhase()) {
			Super::onRpcFrameReceived(std::move(frame));
			return;
		}
		if(cp::RpcMessage::isRequest(frame.meta)) {
		}
		else if(cp::RpcMessage::isResponse(frame.meta)) {
			int rq_id = cp::RpcMessage::requestId(frame.meta).toInt();
			if(rq_id == m_connectionState.pingRqId) {
				m_connectionState.pingRqId = 0;
				return;
			}
		}
		BrokerApp::instance()->onRpcFrameReceived(connectionId(), std::move(frame));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}

int MasterBrokerConnection::connectionId() const
{
	return Super::connectionId();
}
}
