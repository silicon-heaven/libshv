#include "socketrpcconnection.h"
#include "socket.h"

#include <shv/coreqt/log.h>

#include <shv/chainpack/rpcmessage.h>

#include <shv/core/exception.h>

#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTcpSocket>
#include <QHostAddress>

//#define DUMP_DATA_FILE

#ifdef DUMP_DATA_FILE
#include <QFile>
#endif

#define logRpcData() shvCMessage("RpcData")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

SocketRpcConnection::SocketRpcConnection(QObject *parent)
	: QObject(parent)
{
	//shvDebug() << __FUNCTION__;
	Rpc::registerQtMetaTypes();
#ifdef DUMP_DATA_FILE
	QFile *f = new QFile("/tmp/rpc.dat", this);
	f->setObjectName("DUMP_DATA_FILE");
	if(!f->open(QFile::WriteOnly)) {
		shvError() << "cannot open file" << f->fileName() << "for write";
		delete f;
	}
	shvInfo() << "Dumping RPC stream to file:" << f->fileName();
#endif
}

SocketRpcConnection::~SocketRpcConnection()
{
	//shvDebug() << __FUNCTION__;
	abortSocket();
	SHV_SAFE_DELETE(m_socket);
}

void SocketRpcConnection::setSocket(Socket *socket)
{
	socket->setParent(nullptr);
	//shvDebug() << "setSocket" << socket << "pushing socket from thread:" << socket->thread() << "to:" << this->thread();
	//connect(socket, &Socket::destroyed, [socket]() {
	//	shvDebug() << socket << "destroyed";
	//});
	m_socket = socket;
	connect(socket, &Socket::sslErrors, this, &SocketRpcConnection::sslErrors);
	connect(socket, &Socket::error, this, [this](QAbstractSocket::SocketError socket_error) {
		shvWarning() << "Socket error:" << socket_error << m_socket->errorString();
		emit socketError(m_socket->errorString());
		// we are not closing sockets at socket error, do not know why
		if(socket_error == QAbstractSocket::HostNotFoundError) {
			//m_socket->close();
		}
	});
	connect(socket, &Socket::readyRead, this, &SocketRpcConnection::onReadyRead, Qt::QueuedConnection);
	// queued connection here is to write data in next event loop, not directly when previous chunk is written
	// possibly not needed, its my feeling to do it this way
	connect(socket, &Socket::bytesWritten, this, &SocketRpcConnection::onBytesWritten, Qt::QueuedConnection);
	connect(socket, &Socket::connected, this, [this]() {
		shvDebug() << this << "Socket connected!!!";
		//shvWarning() << (peerAddress().toStdString() + ':' + std::to_string(peerPort()));
		emit socketConnectedChanged(true);
	});
	connect(socket, &Socket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
		shvDebug() << this << "Socket state changed" << (int)state;
	});
	connect(socket, &Socket::disconnected, this, [this]() {
		shvDebug() << this << "Socket disconnected!!!";
		emit socketConnectedChanged(false);
	});
}

Socket *SocketRpcConnection::socket()
{
	if(!m_socket)
		SHV_EXCEPTION("Socket is NULL!");
	return m_socket;
}

bool SocketRpcConnection::isSocketConnected() const
{
	return m_socket && m_socket->state() == QTcpSocket::ConnectedState;
}

void SocketRpcConnection::ignoreSslErrors()
{
	m_socket->ignoreSslErrors();
}

void SocketRpcConnection::connectToHost(const QUrl &url)
{
	socket()->connectToHost(url);
}

void SocketRpcConnection::onReadyRead()
{
	QByteArray ba = socket()->readAll();
#ifdef DUMP_DATA_FILE
	QFile *f = findChild<QFile*>("DUMP_DATA_FILE");
	if(f) {
		f->write(ba.constData(), ba.length());
		f->flush();
	}
#endif
	onBytesRead(ba.toStdString());
}

void SocketRpcConnection::onBytesWritten()
{
	logRpcData() << "onBytesWritten()";
	enqueueDataToSend(MessageData());
}

void SocketRpcConnection::onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val)
{
	emit rpcValueReceived(rpc_val);
}

void SocketRpcConnection::onParseDataException(const chainpack::ParseException &e)
{
	if(m_socket)
		m_socket->onParseDataException(e);
}

bool SocketRpcConnection::isOpen()
{
	return isSocketConnected();
}

int64_t SocketRpcConnection::writeBytes(const char *bytes, size_t length)
{
	//shvLogFuncFrame();
	return socket()->write(bytes, length);
}

void SocketRpcConnection::writeMessageBegin()
{
	//shvLogFuncFrame() << "socket:" << m_socket;
	if(m_socket)
		m_socket->writeMessageBegin();
}

void SocketRpcConnection::writeMessageEnd()
{
	//shvLogFuncFrame();
	if(m_socket)
		m_socket->writeMessageEnd();
}

#if 0
namespace {
class ConnectionScope
{
public:
	explicit ConnectionScope(QMetaObject::Connection &c) : m_connection(c) {}
	~ConnectionScope() {if(m_connection) QObject::disconnect(m_connection);}
private:
	QMetaObject::Connection &m_connection;
};
}

void SocketRpcConnection:: sendRpcRequestSync_helper(const shv::chainpack::RpcRequest &request, shv::chainpack::RpcResponse *presponse, int time_out_ms)
{
	namespace cp = shv::chainpack;
	smcDebug() << Q_FUNC_INFO << "timeout ms:" << time_out_ms;
	if(time_out_ms == 0) {
		smcDebug() << "sendMessageSync called with invalid timeout:" << time_out_ms << "," << defaultRpcTimeout() << "msec will be used instead.";
		time_out_ms = defaultRpcTimeout();
	}
	shv::chainpack::RpcResponse resp_msg;
	do {
		const cp::RpcValue msg_id = request.requestId();
		if(!msg_id.isValid()) {
			shvWarning() << "Attempt to send RPC request with ID not set, message will be ignored";
			break;
		}
		smcDebug() << "sending message:" << request.toCpon();
		logRpcSyncCalls() << SND_LOG_ARROW << "SEND SYNC message id:" << msg_id.toCpon() << "msg:" << request.toCpon();
		sendRpcValue(request.value());
		QElapsedTimer tm_elapsed;
		tm_elapsed.start();
		QEventLoop eloop;
		QMetaObject::Connection lambda_connection;
		lambda_connection = connect(this, &SocketRpcConnection::rpcValueReceived, [&eloop, &resp_msg, &lambda_connection, msg_id](const shv::chainpack::RpcValue &msg_val)
		{
			shv::chainpack::RpcMessage msg(msg_val);
			smcDebug() << &eloop << "New RPC message id:" << msg.requestId();
			if(msg.requestId() == msg_id) {
				QObject::disconnect(lambda_connection);
				lambda_connection = QMetaObject::Connection();
				resp_msg = msg;
				smcDebug() << "\tID MATCH stopping event loop ..." << &eloop;
				eloop.quit();
			}
			else if(msg.requestId() == 0) {
				//logRpcSyncCalls() << "<=== RECEIVE NOTIFY while waiting for SYNC response:" << msg.jsonRpcMessage().toString();
			}
			else {
				logRpcSyncCalls() << "RECV other message while waiting for SYNC response id:" << msg.requestId().toCpon() << "json:" << msg.toCpon();
			}
		});
		ConnectionScope cscp(lambda_connection);
		if(time_out_ms > 0)
			QTimer::singleShot(time_out_ms, &eloop, &QEventLoop::quit);

		smcDebug() << "\t entering event loop ..." << &eloop;
		eloop.exec();
		smcDebug() << "\t event loop" << &eloop << "exec exit, message received:" << resp_msg.isValid();
		logRpcSyncCalls() << RCV_LOG_ARROW << "RECV SYNC message id:" << resp_msg.requestId().toCpon() << "msg:" << resp_msg.toCpon();
		int elapsed = (int)tm_elapsed.elapsed();
		if(elapsed >= time_out_ms) {
			cp::RpcValue::String err_msg = "Receive message timeout after: " + shv::chainpack::Utils::toString(elapsed) + " msec!";
			shvError() << err_msg;
			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setRequestId(msg_id);
			resp_msg.setError(err);
		}
		if(!resp_msg.isValid()) {
			cp::RpcValue::String err_msg = "Invalid mesage returned from sync call!";
			shvError() << err_msg;

			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setRequestId(msg_id);
			resp_msg.setError(err);
		}
	} while(false);
	if(presponse)
		*presponse = resp_msg;
}
#endif

void SocketRpcConnection::closeSocket()
{
	if(m_socket) {
		m_socket->close();
	}
}

void SocketRpcConnection::abortSocket()
{
	if(m_socket) {
		m_socket->abort();
	}
}

std::string SocketRpcConnection::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString().toStdString();
	return std::string();
}

int SocketRpcConnection::peerPort() const
{
	if(m_socket)
		return m_socket->peerPort();
	return -1;
}

}}}
