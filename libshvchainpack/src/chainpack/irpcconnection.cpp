#include <shv/chainpack/irpcconnection.h>

#include <necrolog.h>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)

namespace shv::chainpack {

IRpcConnection::IRpcConnection()
	: m_connectionId(nextConnectionId())
{
}

IRpcConnection::~IRpcConnection() = default;

void IRpcConnection::sendSignal(std::string method, const RpcValue &params)
{
	sendShvSignal(std::string(), std::move(method), params);
}

void IRpcConnection::sendShvSignal(const std::string &shv_path, std::string method, const RpcValue &params)
{
	RpcRequest rq;
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	rq.setMethod(std::move(method));
	rq.setParams(params);
	sendMessage(rq);
}

[[deprecated("Use sendSignal instead")]] void IRpcConnection::sendNotify(std::string method, const shv::chainpack::RpcValue &params)
{
	sendSignal(method, params);
}

[[deprecated("Use sendShvSignal instead")]] void IRpcConnection::sendShvNotify(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params)
{
	sendShvSignal(shv_path, method, params);
}

void IRpcConnection::sendResponse(const RpcValue &request_id, const RpcValue &result)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendMessage(resp);
}

void IRpcConnection::sendError(const RpcValue &request_id, const RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendMessage(resp);
}

int IRpcConnection::nextRequestId()
{
	static int n = 0;
	return ++n;
}

int IRpcConnection::connectionId() const
{
	return m_connectionId;
}

int IRpcConnection::nextConnectionId()
{
	static int n = 0;
	return ++n;
}

int IRpcConnection::callMethod(const RpcRequest &rq)
{
	RpcRequest _rq(rq);
	int id = rq.requestId().toInt();
	if(id == 0) {
		id = nextRequestId();
		_rq.setRequestId(id);
	}
	sendMessage(rq);
	return id;
}

int IRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params)
{
	return callShvMethod(shv_path, method, params, {});
}

int IRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params, const RpcValue &user_id)
{
	int id = nextRequestId();
	return callShvMethod(id, shv_path, method, params, user_id);
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, std::string method, const RpcValue &params)
{
	return callShvMethod(rq_id, shv_path, method, params, {});
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, std::string method, const RpcValue &params, const RpcValue &user_id)
{
	RpcRequest rq;
	rq.setRequestId(rq_id);
	rq.setMethod(std::move(method));
	if(params.isValid())
		rq.setParams(params);
	if(user_id.isValid())
		rq.setUserId(user_id);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	sendMessage(rq);
	return rq_id;
}

int IRpcConnection::callMethodSubscribe(const std::string &shv_path, std::string method)
{
	int rq_id = nextRequestId();
	return callMethodSubscribe(rq_id, shv_path, method);
}

int IRpcConnection::callMethodSubscribe(int rq_id, const std::string &shv_path, std::string method)
{
	logSubscriptionsD() << "call subscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method;
	return callShvMethod(rq_id
						 , Rpc::DIR_BROKER_APP
						 , Rpc::METH_SUBSCRIBE
						 , RpcValue::Map{
							 {Rpc::PAR_PATH, shv_path},
							 {Rpc::PAR_METHOD, std::move(method)},
						 });
}

int IRpcConnection::callMethodUnsubscribe(const std::string &shv_path, std::string method)
{
	int rq_id = nextRequestId();
	return callMethodUnsubscribe(rq_id, shv_path, method);
}

int IRpcConnection::callMethodUnsubscribe(int rq_id, const std::string &shv_path, std::string method)
{
	logSubscriptionsD() << "call unsubscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method;
	return callShvMethod(rq_id
						 , Rpc::DIR_BROKER_APP
						 , Rpc::METH_UNSUBSCRIBE
						 , RpcValue::Map{
							 {Rpc::PAR_PATH, shv_path},
							 {Rpc::PAR_METHOD, std::move(method)},
						 });
}

} // namespace shv
