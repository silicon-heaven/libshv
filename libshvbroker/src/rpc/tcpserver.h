#pragma once

#include <shv/broker/shvbrokerglobal.h>

#include <shv/core/utils.h>
#include <shv/chainpack/rpcvalue.h>

#include <QTcpServer>

namespace shv::chainpack { class RpcMessage; }

namespace shv::iotqt::rpc {

class ServerConnection;

class SHVBROKER_DECL_EXPORT TcpServer : public QTcpServer
{
	Q_OBJECT

	using Super = QTcpServer;

public:
	explicit TcpServer(QObject *parent = nullptr);
	~TcpServer() override;

	bool start(int port);
	std::vector<int> connectionIds() const;
	ServerConnection* connectionById(int connection_id);
protected:
	virtual ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) = 0;
	void onNewConnection();
	void unregisterConnection(int connection_id);
protected:
	std::map<int, ServerConnection*> m_connections;
};

}

