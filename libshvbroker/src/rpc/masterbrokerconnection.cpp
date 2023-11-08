#include "masterbrokerconnection.h"
#include "../brokerapp.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/string.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvurl.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/deviceappclioptions.h>

namespace cp = shv::chainpack;

namespace shv {
namespace broker {
namespace rpc {

MasterBrokerConnection::MasterBrokerConnection(QObject *parent)
	: Super(parent)
{

}

void MasterBrokerConnection::setOptions(const shv::chainpack::RpcValue &slave_broker_options)
{
	m_options = slave_broker_options;
	if(slave_broker_options.isMap()) {
		const cp::RpcValue::Map &m = slave_broker_options.asMap();

		shv::iotqt::rpc::DeviceAppCliOptions device_opts;

		const cp::RpcValue::Map &server = m.valref("server").asMap();
		device_opts.setServerHost(server.value("host", "localhost").asString());
		device_opts.setServerPort(server.value("port", shv::chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED).toInt());
		device_opts.setServerSecurityType(server.value("securityType", "none").asString());
		device_opts.setServerPeerVerify(server.value("peerVerify", true).toBool());

		const cp::RpcValue::Map &login = m.valref(cp::Rpc::KEY_LOGIN).asMap();
		for(const std::string key : {"user", "password", "passwordFile", "type"}) {
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
		//device_opts.dump();
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

void MasterBrokerConnection::sendMasterBrokerIdRequest()
{
	m_masterBrokerIdRqId = nextRequestId();
	callShvMethod(m_masterBrokerIdRqId, ".broker/app", "brokerId", cp::RpcValue());
}

void MasterBrokerConnection::sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< RpcDriver::dataToPrettyCpon(shv::chainpack::RpcMessage::protocolType(meta_data), meta_data, data, 0);
	Super::sendRawData(meta_data, std::move(data));
}

void MasterBrokerConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	Super::sendMessage(rpc_msg);
}

CommonRpcClientHandle::Subscription MasterBrokerConnection::createSubscription(const std::string &shv_path, const std::string &method)
{
	using ServiceProviderPath = shv::core::utils::ShvUrl;
	ServiceProviderPath spp(shv_path);
	if(spp.isServicePath())
		SHV_EXCEPTION("This could never happen by SHV design logic, master broker tries to subscribe service provided path: "  + shv_path);
	return Subscription(masterExportedToLocalPath(shv_path), shv_path, method);
}

std::string MasterBrokerConnection::toSubscribedPath(const Subscription &subs, const std::string &signal_path) const
{
	//Q_UNUSED(subs)
	bool debug = true;
	if(debug) {
		using ServiceProviderPath = shv::core::utils::ShvUrl;
		ServiceProviderPath spp(signal_path);
		if(spp.isServicePath())
			shvWarning() << "Master broker subscription should not have to handle service provider signal:" << signal_path << "subscription:" << subs.subscribedPath;
	}
	return localPathToMasterExported(signal_path);
}

std::string MasterBrokerConnection::masterExportedToLocalPath(const std::string &master_path) const
{
	if(m_exportedShvPath.empty())
		return master_path;
	static const std::string DIR_BROKER = cp::Rpc::DIR_BROKER;
	if(shv::core::utils::ShvPath::startsWithPath(master_path, DIR_BROKER))
		return master_path;
	return shv::core::utils::ShvPath(m_exportedShvPath).appendPath(master_path);
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

void MasterBrokerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, std::string &&msg_data)
{
	logRpcMsg() << RpcDriver::RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocol_type << shv::chainpack::Rpc::protocolTypeToString(protocol_type)
				<< RpcDriver::dataToPrettyCpon(protocol_type, md, msg_data, 0, msg_data.size());
	try {
		if(isInitPhase()) {
			Super::onRpcDataReceived(protocol_type, std::move(md), std::move(msg_data));
			return;
		}
		if(cp::RpcMessage::isRequest(md)) {
			/*
			const std::string &request_shv_path = cp::RpcMessage::shvPath(md).asString();
			shv::core::utils::ShvUrl request_shv_url(request_shv_path);
			std::string shv_path;
			if (shv::core::utils::ShvPath::startsWithPath(request_shv_path, shv::iotqt::node::ShvNode::LOCAL_NODE_HACK)) {
				if (cp::RpcMessage::accessGrant(md).toString() != cp::Rpc::ROLE_ADMIN) {
					shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(md);
					rsp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Insufficient rights for node: " + shv::iotqt::node::ShvNode::LOCAL_NODE_HACK));
					sendMessage(rsp);
					return;
				}
				shv_path = shv::core::utils::ShvPath::midPath(request_shv_path, 1).toString();
				cp::RpcMessage::setShvPath(md, shv_path);
			}
			else if(request_shv_url.isPlain()) {
				shv_path = masterExportedToLocalPath(request_shv_path);
				if (request_shv_path.empty() && cp::RpcMessage::method(md) == cp::Rpc::METH_LS && cp::RpcMessage::accessGrant(md).toString() == cp::Rpc::ROLE_ADMIN) {
					md.setValue(shv::iotqt::node::ShvNode::ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY, true);
				}
				cp::RpcMessage::setShvPath(md, shv_path);
			}
			*/
		}
		else if(cp::RpcMessage::isResponse(md)) {
			int rq_id = cp::RpcMessage::requestId(md).toInt();
			if(rq_id == m_connectionState.pingRqId) {
				m_connectionState.pingRqId = 0;
				return;
			}
			else if (rq_id == m_masterBrokerIdRqId) {
				m_masterBrokerIdRqId = 0;
				cp::RpcValue rpc_val = decodeData(protocol_type, msg_data, 0);
				rpc_val.setMetaData(cp::RpcValue::MetaData(md));
				Q_EMIT masterBrokerIdReceived(cp::RpcResponse(cp::RpcMessage(rpc_val)));
			}
		}
		BrokerApp::instance()->onRpcDataReceived(connectionId(), protocol_type, std::move(md), std::move(msg_data));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}

}}}
