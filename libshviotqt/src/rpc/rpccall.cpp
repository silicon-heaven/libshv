#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/coreqt/log.h>

#include <QTimer>

using namespace shv::chainpack;

namespace shv::iotqt::rpc {

//===================================================
// RpcCall
//===================================================
RpcResponseCallBack::RpcResponseCallBack(ClientConnection *conn, int rq_id, QObject *parent)
	: QObject(parent)
{
	setRequestId(rq_id);
	connect(conn, &ClientConnection::rpcFrameReceived, this, &RpcResponseCallBack::onRpcFrameReceived);
	connect(conn, &ClientConnection::rpcMessageReceived, this, &RpcResponseCallBack::onRpcMessageReceived);
	connect(conn, &ClientConnection::responseMetaReceived, this, &RpcResponseCallBack::onResponseMetaReceived);
	connect(conn, &ClientConnection::dataChunkReceived, this, &RpcResponseCallBack::onDataChunkReceived);
	setTimeout(conn->rpcTimeoutMsec());
}

void RpcResponseCallBack::start()
{
	m_isFinished = false;
	if(!m_timeoutTimer) {
		m_timeoutTimer = new QTimer(this);
		m_timeoutTimer->setSingleShot(true);
		connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
			shv::chainpack::RpcResponse resp;
			resp.setError(shv::chainpack::RpcResponse::Error::create(shv::chainpack::RpcResponse::Error::MethodCallTimeout, "Shv call timeout after: " + std::to_string(m_timeoutTimer->interval()) + " msec."));
			m_isFinished = true;
			if(m_callBackFunction)
				m_callBackFunction(resp);
			else
				emit finished(resp);
			deleteLater();
		});
	}
	m_timeoutTimer->start(timeout());
}

void RpcResponseCallBack::start(int time_out)
{
	setTimeout(time_out);
	start();
}

void RpcResponseCallBack::start(const RpcResponseCallBack::CallBackFunction& cb)
{
	m_callBackFunction = cb;
	start();
}

void RpcResponseCallBack::start(int time_out, const RpcResponseCallBack::CallBackFunction& cb)
{
	setTimeout(time_out);
	start(cb);
}

void RpcResponseCallBack::start(QObject *context, const RpcResponseCallBack::CallBackFunction& cb)
{
	start(timeout(), context, cb);
}

void RpcResponseCallBack::start(int time_out_msec, QObject *context, const RpcResponseCallBack::CallBackFunction& cb)
{
	if(context) {
		connect(context, &QObject::destroyed, this, [this]() {
			m_callBackFunction = nullptr;
			deleteLater();
		});
	}
	start(time_out_msec, cb);
}

void RpcResponseCallBack::abort()
{
	shv::chainpack::RpcResponse resp;
	resp.setError(shv::chainpack::RpcResponse::Error::create(shv::chainpack::RpcResponse::Error::MethodCallCancelled, "Shv call aborted"));
	m_isFinished = true;

	if(m_callBackFunction)
		m_callBackFunction(resp);
	else
		emit finished(resp);
	deleteLater();
}

void RpcResponseCallBack::onRpcFrameReceived()
{
	// reset timeout timer even if frame cannot be parsed to valid RpcMessage
	// frame comming from PLC for example
	if (m_isFinished) {
		return;
	}

	if (m_timeoutTimer && m_responseMetaReceived) {
		m_responseFrameReceived = true;
	}
}

void RpcResponseCallBack::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	if(m_isFinished) {
		return;
	}
	shvLogFuncFrame() << this << msg.toPrettyString();
	if(!msg.isResponse()) {
		return;
	}
	RpcResponse rsp(msg);
	if(rsp.peekCallerId() != 0
		|| rsp.requestId() != requestId()
		|| rsp.delay().has_value()
		) {
		return;
	}
	m_isFinished = true;
	if(m_timeoutTimer) {
		m_timeoutTimer->stop();
	} else {
		shvWarning() << "Callback was not started, time-out functionality cannot be provided!";
	}
	if(m_callBackFunction) {
		m_callBackFunction(rsp);
	} else {
		emit finished(rsp);
	}
	deleteLater();
}

void RpcResponseCallBack::onResponseMetaReceived(int request_id)
{
	if(m_isFinished)
		return;
	if(request_id == requestId()) {
		m_responseMetaReceived = true;
		if(m_timeoutTimer) {
			// response is being received
			m_timeoutTimer->start();
		}
	}
}

void RpcResponseCallBack::onDataChunkReceived()
{
	if(m_isFinished)
		return;

	if (m_responseFrameReceived) {
		return;
	}

	if(m_timeoutTimer && m_responseMetaReceived) {
		// response is being received
		m_timeoutTimer->start();
	}
}

//===================================================
// RpcCall
//===================================================
RpcCall::RpcCall(ClientConnection *connection)
	: m_rpcConnection(connection)
{
	connect(this, &RpcCall::maybeResult, this, [this](const ::shv::chainpack::RpcValue &_result, const ::shv::chainpack::RpcError &_error) {
		if(_error.isValid())
			emit error(_error);
		else
			emit result(_result);
	});

//	connect(this, &RpcCall::maybeResult, this, &QObject::deleteLater, Qt::QueuedConnection);
}

RpcCall *RpcCall::createSubscriptionRequest(ClientConnection *connection, const QString &shv_path, const QString &signal, const QString &source)
{
	RpcCall *rpc = create(connection);
	const auto &[subscribe_path, params] = shv::chainpack::IRpcConnection::makeSubscribeParams(connection->shvApiVersion(), shv_path.toStdString(), signal.toStdString(), source.toStdString());
	rpc->setShvPath(subscribe_path)
			->setMethod(Rpc::METH_SUBSCRIBE)
			->setParams(params);
	return rpc;
}

RpcCall *RpcCall::create(ClientConnection *connection)
{
	return new RpcCall(connection);
}

RpcCall *RpcCall::setRequestId(int rq_id)
{
	m_requestId = rq_id;
	return this;
}

RpcCall *RpcCall::setShvPath(const std::string &shv_path)
{
	m_shvPath = shv_path;
	return this;
}

RpcCall *RpcCall::setShvPath(const char *shv_path)
{
	return setShvPath(std::string(shv_path));
}

RpcCall *RpcCall::setShvPath(const QString &shv_path)
{
	return setShvPath(shv_path.toStdString());
}

RpcCall *RpcCall::setShvPath(const QStringList &shv_path)
{
	return setShvPath(shv_path.join('/'));
}

RpcCall *RpcCall::setMethod(const std::string &method)
{
	m_method = method;
	return this;
}

RpcCall *RpcCall::setMethod(const char *method)
{
	return setMethod(std::string(method));
}

RpcCall *RpcCall::setMethod(const QString &method)
{
	m_method = method.toStdString();
	return this;
}

RpcCall *RpcCall::setParams(const RpcValue &params)
{
	m_params = params;
	return this;
}

RpcCall* RpcCall::setTimeout(int timeout)
{
	m_timeout = timeout;
	return this;
}

RpcCall *RpcCall::setUserId(const chainpack::RpcValue &user_id)
{
	m_userId = user_id;
	return this;
}

std::string RpcCall::shvPath() const
{
	return m_shvPath;
}

int RpcCall::start()
{
	if(m_rpcConnection.isNull()) {
		emit maybeResult({}, RpcError("RPC connection is NULL"));
		deleteLater();
		return 0;
	}
	if(!m_rpcConnection->isBrokerConnected()) {
		emit maybeResult({}, RpcError("RPC connection is not open"));
		deleteLater();
		return 0;
	}
	int rq_id = m_requestId > 0? m_requestId: m_rpcConnection->nextRequestId();
	auto *cb = new RpcResponseCallBack(m_rpcConnection, rq_id, this);
	if (m_timeout) {
		cb->setTimeout(m_timeout);
	}
	cb->start(this, [this](const RpcResponse &resp) {
		if (resp.isSuccess()) {
			emit maybeResult(resp.result(), {});
		}
		else {
			emit maybeResult(RpcValue(), resp.error());
		}
		deleteLater();
	});
	m_rpcConnection->callShvMethod(rq_id, m_shvPath, m_method, m_params, m_userId);
	return rq_id;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QFuture<chainpack::RpcValue> RpcCall::intoFuture() &
{
	auto fut = QtFuture::connect(this, &RpcCall::maybeResult)
		.then(this, [] (std::tuple<const shv::chainpack::RpcValue&, const shv::chainpack::RpcError&> resultOrError) {
			const auto& [result, error] = resultOrError;
			if (error.isValid()) {
				throw shv::chainpack::RpcException{error.code(), error.message()};
			}

			return result;
		});
	this->start();
	return fut;
}
#endif
} // namespace shv
