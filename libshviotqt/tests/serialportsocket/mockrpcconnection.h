#pragma once

#include <shv/iotqt/rpc/socketrpcconnection.h>

class MockRpcConnection : public shv::iotqt::rpc::SocketRpcConnection
{
	Q_OBJECT
public:
	explicit MockRpcConnection(QObject *parent = nullptr);

	void close() override;
	void abort() override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
};

