#include <shv/iotqt/rpc/socketrpcconnection.h>
#include <shv/iotqt/rpc/socket.h>

#include <shv/coreqt/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/exception.h>

#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTcpSocket>
#include <QHostAddress>
#include <QCoreApplication>

//#define DUMP_DATA_FILE

#ifdef DUMP_DATA_FILE
#include <QFile>
#endif

#define logRpcData() shvCMessage("RpcData")

namespace shv::iotqt::rpc {

SocketRpcConnection::SocketRpcConnection(QObject *parent)
	: QObject(parent)
{
	shv::coreqt::rpc::registerQtMetaTypes();
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
	abortSocket();
	SHV_SAFE_DELETE(m_socket);
}

void SocketRpcConnection::setSocket(Socket *socket)
{
	socket->setParent(nullptr);
	m_socket = socket;
	connect(socket, &Socket::sslErrors, this, &SocketRpcConnection::sslErrors);
	connect(socket, &Socket::error, this, [this](QAbstractSocket::SocketError /*socket_error*/) {
		shvWarning() << "Socket error:" << m_socket->errorString();
		emit socketError(m_socket->errorString());
	});
	bool is_test_run = QCoreApplication::instance() == nullptr;
	connect(socket, &Socket::readyRead, this, &SocketRpcConnection::onReadyRead, is_test_run? Qt::AutoConnection: Qt::QueuedConnection);
	connect(socket, &Socket::readyRead, this, &SocketRpcConnection::socketDataReadyRead, is_test_run? Qt::AutoConnection: Qt::QueuedConnection);
	// queued connection here is to write data in next event loop, not directly when previous chunk is written
	// possibly not needed, its my feeling to do it this way
	connect(socket, &Socket::bytesWritten, this, &SocketRpcConnection::onBytesWritten, is_test_run? Qt::AutoConnection: Qt::QueuedConnection);
	connect(socket, &Socket::connected, this, [this]() {
		shvDebug() << this << "Socket connected!!!";
		emit socketConnectedChanged(true);
	});
	connect(socket, &Socket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
		shvDebug() << this << "Socket state changed" << static_cast<int>(state);
	});
	connect(socket, &Socket::disconnected, this, [this]() {
		shvDebug() << this << "Socket disconnected!!!";
		emit socketConnectedChanged(false);
	});
	connect(socket, &Socket::socketReset, this, &SocketRpcConnection::clearSendBuffers);
}

bool SocketRpcConnection::hasSocket() const
{
	return m_socket != nullptr;
}

void SocketRpcConnection::setProtocolTypeAsInt(int v)
{
	shv::chainpack::RpcDriver::setProtocolType(static_cast<shv::chainpack::Rpc::ProtocolType>(v));
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
	return socket()->write(bytes, static_cast<qint64>(length));
}

void SocketRpcConnection::writeMessageBegin()
{
	if(m_socket)
		m_socket->writeMessageBegin();
}

void SocketRpcConnection::writeMessageEnd()
{
	if(m_socket)
		m_socket->writeMessageEnd();
}

Q_SLOT void SocketRpcConnection::sendRpcValue(const shv::chainpack::RpcValue &rpc_val)
{
	shv::chainpack::RpcDriver::sendRpcValue(rpc_val);
}

void SocketRpcConnection::clearSendBuffers()
{
	shv::chainpack::RpcDriver::clearSendBuffers();
}

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

}
