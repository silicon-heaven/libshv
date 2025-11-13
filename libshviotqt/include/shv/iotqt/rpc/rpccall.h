#pragma once

#include <shv/iotqt/shviotqtglobal.h>

#include <shv/coreqt/rpc.h>

#include <shv/core/utils.h>
#include <shv/chainpack/rpc.h>
#include <shv/chainpack/rpcmessage.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QFuture>
#endif
#include <QObject>
#include <QPointer>

#include <functional>

class QTimer;

namespace shv {

namespace chainpack { class RpcMessage; class RpcResponse; class RpcError; }

namespace iotqt::rpc {

class ClientConnection;

class SHVIOTQT_DECL_EXPORT RpcResponseCallBack : public QObject
{
	Q_OBJECT
public:
	using CallBackFunction = std::function<void (const shv::chainpack::RpcResponse &rsp)>;

	SHV_FIELD_IMPL(int, r, R, equestId)
	SHV_FIELD_IMPL2(int, t, T, imeout, 1*60*1000)

public:
	explicit RpcResponseCallBack(shv::iotqt::rpc::ClientConnection *conn, int rq_id, QObject *parent = nullptr);

	Q_SIGNAL void finished(const shv::chainpack::RpcResponse &response);

	void start();
	void start(int time_out);
	void start(const CallBackFunction& cb);
	void start(int time_out, const CallBackFunction& cb);
	void start(QObject *context, const CallBackFunction& cb);
	void start(int time_out_msec, QObject *context, const CallBackFunction& cb);
	void abort();
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onRpcFrameReceived();
private:
	void onResponseMetaReceived(int request_id);
	void onDataChunkReceived();
private:
	CallBackFunction m_callBackFunction;
	QTimer *m_timeoutTimer = nullptr;
	bool m_responseMetaReceived = false;
	bool m_responseFrameReceived = false;
	bool m_isFinished = false;
};

class SHVIOTQT_DECL_EXPORT RpcCall : public QObject
{
	Q_OBJECT
public:
	static RpcCall* createSubscriptionRequest(::shv::iotqt::rpc::ClientConnection *connection, const QString &shv_path, const QString &method, const QString &source = QString(shv::chainpack::Rpc::METH_GET));
	static RpcCall* create(::shv::iotqt::rpc::ClientConnection *connection);

	RpcCall* setRequestId(int rq_id);
	RpcCall* setShvPath(const std::string &shv_path);
	RpcCall* setShvPath(const char *shv_path);
	RpcCall* setShvPath(const QString &shv_path);
	RpcCall* setShvPath(const QStringList &shv_path);
	RpcCall* setMethod(const std::string &method);
	RpcCall* setMethod(const char *method);
	RpcCall* setMethod(const QString &method);
	RpcCall* setParams(const ::shv::chainpack::RpcValue &params);
	RpcCall* setTimeout(int timeout);
	RpcCall* setUserId(const ::shv::chainpack::RpcValue &user_id);

	std::string shvPath() const;
	int start();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QFuture<chainpack::RpcValue> intoFuture() &;
#endif

	Q_SIGNAL void maybeResult(const ::shv::chainpack::RpcValue &result, const ::shv::chainpack::RpcError &error);
	Q_SIGNAL void result(const ::shv::chainpack::RpcValue &result);
	Q_SIGNAL void error(const ::shv::chainpack::RpcError &error);
private:
	RpcCall(::shv::iotqt::rpc::ClientConnection *connection);
private:
	QPointer<::shv::iotqt::rpc::ClientConnection> m_rpcConnection;
	std::string m_shvPath;
	std::string m_method;
	shv::chainpack::RpcValue m_params;
	int m_timeout = 0;
	shv::chainpack::RpcValue m_userId;
	int m_requestId = 0;
};

} // namespace iotqt::rpc

} // namespace shv
