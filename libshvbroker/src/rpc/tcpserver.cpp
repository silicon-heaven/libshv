#include "serverconnection.h"
#include "tcpserver.h"

#include <shv/coreqt/log.h>

#include <QTcpSocket>

namespace shv::iotqt::rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{
	connect(this, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
	shvMessage() << "Destroying SHV TcpServer";
}

bool TcpServer::start(int port)
{
	shvMessage() << "start listenning on port:" << port;
	if (!listen(QHostAddress::AnyIPv4, static_cast<quint16>(port))) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace() << "TCP RPC server is listenning on " << serverAddress().toString() << ":" << serverPort();
	return true;
}

std::vector<int> TcpServer::connectionIds() const
{
	std::vector<int> ret;
	for(const auto &pair : m_connections)
		ret.push_back(pair.first);
	return ret;
}

ServerConnection *TcpServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}

void TcpServer::onNewConnection()
{
	QTcpSocket *sock = nextPendingConnection();
	if(sock) {
		shvInfo().nospace() << "client connected: " << sock->peerAddress().toString() << ':' << sock->peerPort() << " @ " << serverPort();// << "socket:" << sock << sock->socketDescriptor() << "state:" << sock->state();
		ServerConnection *c = createServerConnection(sock, this);
		c->setConnectionName(sock->peerAddress().toString().toStdString() + ':' + std::to_string(sock->peerPort()));
		m_connections[c->connectionId()] = c;
		connect(c, &ServerConnection::aboutToBeDeleted, this, &TcpServer::unregisterConnection);
	}
}

void TcpServer::unregisterConnection(int connection_id)
{
	m_connections.erase(connection_id);
}

}
