#pragma once

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/accessgrant.h>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT IRpcConnection
{
public:
	static constexpr int DEFAULT_RPC_BROKER_PORT_NONSECURED = 3755;
	static constexpr int DEFAULT_RPC_BROKER_PORT_SECURED = 37555;
	static constexpr int DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_NONSECURED = 3777;
	static constexpr int DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_SECURED = 3778;

	using LoginType = UserLogin::LoginType;
public:
	IRpcConnection();
	virtual ~IRpcConnection();

	virtual int connectionId() const;

	virtual void close() = 0;
	virtual void abort() = 0;

	virtual void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) = 0;
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;

	int nextRequestId();

	void sendSignal(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendShvSignal(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	[[deprecated("Use sendSignal instead")]] void sendNotify(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	[[deprecated("Use sendShvSignal instead")]] void sendShvNotify(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());

	void sendResponse(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcValue &result);
	void sendError(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcResponse::Error &error);
	int callMethod(const shv::chainpack::RpcRequest &rq);
	int callShvMethod(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	int callShvMethod(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id);
	int callShvMethod(int rq_id, const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	int callShvMethod(int rq_id, const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id);
	int callMethodSubscribe(const std::string &shv_path, std::string method);
	int callMethodSubscribe(int rq_id, const std::string &shv_path, std::string method);
	int callMethodUnsubscribe(const std::string &shv_path, std::string method);
	int callMethodUnsubscribe(int rq_id, const std::string &shv_path, std::string method);
protected:
	static int nextConnectionId();
protected:
	int m_connectionId;
};

} // namespace chainpack
} // namespace shv
