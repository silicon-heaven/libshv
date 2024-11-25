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
	sendRpcMessage(rq);
}

void IRpcConnection::sendResponse(const RpcValue &request_id, const RpcValue &result)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendRpcMessage(resp);
}

void IRpcConnection::sendError(const RpcValue &request_id, const RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendRpcMessage(resp);
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
	sendRpcMessage(rq);
	return id;
}

int IRpcConnection::callShvMethod(const std::string &shv_path, const std::string& method, const RpcValue &params)
{
	return callShvMethod(shv_path, method, params, {});
}

int IRpcConnection::callShvMethod(const std::string &shv_path, const std::string& method, const RpcValue &params, const RpcValue &user_id)
{
	int id = nextRequestId();
	return callShvMethod(id, shv_path, method, params, user_id);
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, const std::string& method, const RpcValue &params)
{
	return callShvMethod(rq_id, shv_path, method, params, {});
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, const std::string& method, const RpcValue &params, const RpcValue &user_id)
{
	RpcRequest rq;
	rq.setRequestId(rq_id);
	rq.setMethod(method);
	if(params.isValid())
		rq.setParams(params);
	if(user_id.isValid())
		rq.setUserId(user_id);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	sendRpcMessage(rq);
	return rq_id;
}

int IRpcConnection::callMethodSubscribeGlob(const std::string& glob)
{
	logSubscriptionsD() << "call subscribe for connection id:" << connectionId() << "glob:" << glob;
	int rq_id = nextRequestId();
	if(m_shvApiVersion == ShvApiVersion::V3) {
		return callShvMethod(rq_id
							 , Rpc::DIR_BROKER_CURRENTCLIENT
							 , Rpc::METH_SUBSCRIBE
							 , RpcList{ glob, nullptr }
							 );
	}
	nError() << "Glob subscribe is supported on SHV3 devices only.";
	return 0;
}

int IRpcConnection::callMethodSubscribe(const std::string &shv_path, const std::string& method, const std::string& source)
{
	int rq_id = nextRequestId();
	return callMethodSubscribe(rq_id, shv_path, method, source);
}

int IRpcConnection::callMethodSubscribe(int rq_id, const std::string &shv_path, const std::string& method, const std::string& source)
{
	logSubscriptionsD() << "call subscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method << "source:" << source;
	if(m_shvApiVersion == ShvApiVersion::V3) {
		auto path = shv_path + "/**"; // make V3 glob from V2 path
		auto ri = path + ':' + (source.empty()? "*": source) + ':' + (method.empty()? "*": method);
		return callShvMethod(rq_id
							 , Rpc::DIR_BROKER_CURRENTCLIENT
							 , Rpc::METH_SUBSCRIBE
							 , RpcList{ ri, nullptr }
							 );
	}
	return callShvMethod(rq_id
						 , Rpc::DIR_BROKER_APP
						 , Rpc::METH_SUBSCRIBE
						 , RpcValue::Map{
							 {Rpc::PAR_PATH, shv_path},
							 {Rpc::PAR_METHOD, method},
							 {Rpc::PAR_SOURCE, source},
						 });
}

int IRpcConnection::callMethodUnsubscribe(const std::string &shv_path, const std::string& method, const std::string& source)
{
	int rq_id = nextRequestId();
	return callMethodUnsubscribe(rq_id, shv_path, method, source);
}

int IRpcConnection::callMethodUnsubscribe(int rq_id, const std::string &shv_path, const std::string& method, const std::string& source)
{
	logSubscriptionsD() << "call unsubscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method << "source:" << source;
	if(m_shvApiVersion == ShvApiVersion::V3) {
		auto path = shv_path + "/**";
		auto ri = path + ':' + (source.empty()? "*": source) + ':' + (method.empty()? "*": method);
		return callShvMethod(rq_id
							 , Rpc::DIR_BROKER_CURRENTCLIENT
							 , Rpc::METH_UNSUBSCRIBE
							 , ri
							 );
	}
	return callShvMethod(rq_id
						 , Rpc::DIR_BROKER_APP
						 , Rpc::METH_UNSUBSCRIBE
						 , RpcValue::Map{
							 {Rpc::PAR_PATH, shv_path},
							 {Rpc::PAR_METHOD, method},
							 {Rpc::PAR_SOURCE, source},
						 });
}

} // namespace shv
