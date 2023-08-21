#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

namespace rpc { class BrokerTcpServer; class ClientConnectionOnBroker; }

class ClientShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ClientShvNode(const std::string &node_id, rpc::ClientConnectionOnBroker *conn, shv::iotqt::node::ShvNode *parent = nullptr);
	~ClientShvNode() override;

	rpc::ClientConnectionOnBroker * connection() const;
	QList<rpc::ClientConnectionOnBroker *> connections() const;

	void addConnection(rpc::ClientConnectionOnBroker *conn);
	void removeConnection(rpc::ClientConnectionOnBroker *conn);

	void handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
private:
	QList<rpc::ClientConnectionOnBroker *> m_connections;
};

class MasterBrokerShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	MasterBrokerShvNode(shv::iotqt::node::ShvNode *parent = nullptr);
	~MasterBrokerShvNode() override;
};

}}
