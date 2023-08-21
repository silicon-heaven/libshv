#include "clientshvnode.h"

#include "rpc/clientconnectiononbroker.h"

#include <shv/coreqt/log.h>

namespace shv::broker {

//======================================================================
// ClientShvNode
//======================================================================
ClientShvNode::ClientShvNode(const std::string &node_id, rpc::ClientConnectionOnBroker *conn, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "Creating client node:" << this << nodeId() << "connection:" << conn->connectionId();
	addConnection(conn);
}

ClientShvNode::~ClientShvNode()
{
	shvInfo() << "Destroying client node:" << this << nodeId();// << "connections:" << [this]() { std::string s; for(auto c : m_connections) s += std::to_string(c->connectionId()) + " "; return s;}();
}

rpc::ClientConnectionOnBroker * ClientShvNode::connection() const
{
	return m_connections.value(0);
}

QList<rpc::ClientConnectionOnBroker *> ClientShvNode::connections() const
{
	return m_connections;
}

void ClientShvNode::addConnection(rpc::ClientConnectionOnBroker *conn)
{
	// prefere new connections, old one might not work
	m_connections.insert(0, conn);
	connect(conn, &rpc::ClientConnectionOnBroker::destroyed, this, [this, conn]() {removeConnection(conn);});
}

void ClientShvNode::removeConnection(rpc::ClientConnectionOnBroker *conn)
{
	m_connections.removeOne(conn);
	if(m_connections.isEmpty() && ownChildren().isEmpty())
		deleteLater();
}

void ClientShvNode::handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data)
{
	rpc::ClientConnectionOnBroker *conn = connection();
	if(conn)
		conn->sendRawData(meta, std::move(data));
}

shv::chainpack::RpcValue ClientShvNode::hasChildren(const StringViewList &shv_path)
{
	Q_UNUSED(shv_path)
	return nullptr;
}

//======================================================================
// MasterBrokerShvNode
//======================================================================
MasterBrokerShvNode::MasterBrokerShvNode(ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating master broker connection node:" << this;
}

MasterBrokerShvNode::~MasterBrokerShvNode()
{
	shvInfo() << "Destroying master broker connection node:" << this;
}

}
