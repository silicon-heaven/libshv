#include "mockrpcconnection.h"

#include <shv/iotqt/rpc/socket.h>

MockRpcConnection::MockRpcConnection(QObject *parent)
	: shv::iotqt::rpc::SocketRpcConnection{parent}
{
}

void MockRpcConnection::close()
{
	socket()->close();
}

void MockRpcConnection::abort()
{
	socket()->abort();
}

void MockRpcConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	sendMessage(rpc_msg.value());
}

void MockRpcConnection::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	emit rpcMessageReceived(msg);
}

