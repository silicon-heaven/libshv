#include "rpcresponsecallback.h"
#include "clientconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/coreqt/log.h>

#include <QTimer>

namespace cp = shv::chainpack;

namespace shv::iotqt::rpc {

//===================================================
// RpcCall
//===================================================
RpcResponseCallBack::RpcResponseCallBack(ClientConnection *conn, int rq_id, QObject *parent)
	: QObject(parent)
{
	setRequestId(rq_id);
	connect(conn, &ClientConnection::rpcMessageReceived, this, &RpcResponseCallBack::onRpcMessageReceived);
	connect(conn, &ClientConnection::socketDataReadyRead, this, &RpcResponseCallBack::onConnectionDataReadyRead);
	setTimeout(shv::iotqt::rpc::ClientConnection::defaultRpcTimeoutMsec());
}

void RpcResponseCallBack::onConnectionDataReadyRead()
{
	// restart timeout timer everytime when some data are read
	// jumbo messages can time-out withou this simply because
	// they are received too long
	if(m_timeoutTimer && m_timeoutTimer->isActive()) {
		m_timeoutTimer->start();
	}
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

void RpcResponseCallBack::start(RpcResponseCallBack::CallBackFunction cb)
{
	m_callBackFunction = cb;
	start();
}

void RpcResponseCallBack::start(int time_out, RpcResponseCallBack::CallBackFunction cb)
{
	setTimeout(time_out);
	start(cb);
}

void RpcResponseCallBack::start(QObject *context, RpcResponseCallBack::CallBackFunction cb)
{
	start(timeout(), context, cb);
}

void RpcResponseCallBack::start(int time_out_msec, QObject *context, RpcResponseCallBack::CallBackFunction cb)
{
	if(context) {
		connect(context, &QObject::destroyed, this, [this]() {
			if (m_isFinished)
				return;
			m_callBackFunction = nullptr;
			m_isFinished = true;
			deleteLater();
		});
	}
	start(time_out_msec, cb);
}

void RpcResponseCallBack::abort()
{
	if (m_isFinished)
		return;
	shv::chainpack::RpcResponse resp;
	resp.setError(shv::chainpack::RpcResponse::Error::create(shv::chainpack::RpcResponse::Error::MethodCallCancelled, "Shv call aborted"));
	m_isFinished = true;

	if(m_callBackFunction)
		m_callBackFunction(resp);
	else
		emit finished(resp);
	deleteLater();
}

void RpcResponseCallBack::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	if(m_isFinished)
		return;
	shvLogFuncFrame() << this << msg.toPrettyString();
	if(!msg.isResponse())
		return;
	cp::RpcResponse rsp(msg);
	if(rsp.peekCallerId() != 0 || !(rsp.requestId() == requestId()))
		return;
	m_isFinished = true;
	if(m_timeoutTimer)
		m_timeoutTimer->stop();
	else
		shvWarning() << "Timer was not started, time-out functionality cannot be provided!";
	if(m_callBackFunction)
		m_callBackFunction(rsp);
	else
		emit finished(rsp);
	deleteLater();
}

//===================================================
// RpcCall
//===================================================
RpcCall::RpcCall(ClientConnection *connection)
	: m_rpcConnection(connection)
{
	connect(this, &RpcCall::maybeResult, this, [this](const ::shv::chainpack::RpcValue &_result, const QString &_error) {
		if(_error.isEmpty())
			emit result(_result);
		else
			emit error(_error);
	});

	connect(this, &RpcCall::maybeResult, this, &QObject::deleteLater, Qt::QueuedConnection);
}

RpcCall *RpcCall::createSubscriptionRequest(ClientConnection *connection, const QString &shv_path, const QString &method)
{
	RpcCall *rpc = create(connection);
	rpc->setShvPath(cp::Rpc::DIR_BROKER_APP)
			->setMethod(cp::Rpc::METH_SUBSCRIBE)
			->setParams(cp::RpcValue::Map {
							{cp::Rpc::PAR_PATH, shv_path.toStdString()},
							{cp::Rpc::PAR_METHOD, method.toStdString()},
						});
	return rpc;
}

RpcCall *RpcCall::create(ClientConnection *connection)
{
	return new RpcCall(connection);
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

RpcCall *RpcCall::setParams(const cp::RpcValue &params)
{
	m_params = params;
	return this;
}

RpcCall *RpcCall::setUserId(const chainpack::RpcValue &user_id)
{
	m_userId = user_id;
	return this;
}

void RpcCall::start()
{
	if(m_rpcConnection.isNull()) {
		emit maybeResult(cp::RpcValue(), "RPC connection is NULL");
		return;
	}
	if(!m_rpcConnection->isBrokerConnected()) {
		emit maybeResult(cp::RpcValue(), "RPC connection is not open");
		return;
	}
	int rq_id = m_rpcConnection->nextRequestId();
	auto *cb = new RpcResponseCallBack(m_rpcConnection, rq_id);
	if (m_timeout) {
		cb->setTimeout(m_timeout);
	}
	cb->start(this, [this](const cp::RpcResponse &resp) {
		if (resp.isSuccess()) {
			emit maybeResult(resp.result(), QString());
		}
		else {
			emit maybeResult(cp::RpcValue(), QString::fromStdString(resp.errorString()));
		}
	});
	m_rpcConnection->callShvMethod(rq_id, m_shvPath, m_method, m_params, m_userId);
}

} // namespace shv
