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

#define logRpcData() shvCMessage("RpcData")

namespace shv::iotqt::rpc {

SocketRpcConnection::SocketRpcConnection(QObject *parent)
	: QObject(parent)
{
	shv::coreqt::rpc::registerQtMetaTypes();
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
	connect(socket, &Socket::responseMetaReceived, this, &SocketRpcConnection::responseMetaReceived);
	connect(socket, &Socket::dataChunkReceived, this, &SocketRpcConnection::dataChunkReceived);
	connect(socket, &Socket::sslErrors, this, &SocketRpcConnection::sslErrors);
	connect(socket, &Socket::error, this, &SocketRpcConnection::onSocketError);

	bool is_test_run = QCoreApplication::instance() == nullptr;
	connect(socket, &Socket::readyRead, this, &SocketRpcConnection::onReadyRead, is_test_run? Qt::AutoConnection: Qt::QueuedConnection);
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
	//connect(socket, &Socket::socketReset, this, &SocketRpcConnection::clearSendBuffers);
}

bool SocketRpcConnection::hasSocket() const
{
	return m_socket != nullptr;
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
	auto frames = socket()->takeFrames();
	for (auto begin = std::make_move_iterator(frames.begin()), end = std::make_move_iterator(frames.end()); begin != end; ++begin) {
		processRpcFrame(*begin);
	}
}

void SocketRpcConnection::onSocketError(QAbstractSocket::SocketError socket_error)
{
	shvWarning() << "Socket error:" << socket_error << m_socket->errorString();
	if (socket()->isOpen()) {
		// close socket to release file descriptor for reopen
		// needed especially in case of serial port connection
		closeSocket();
	}
	emit socketError(m_socket->errorString());
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

void SocketRpcConnection::writeFrameData(const std::string &frame_data)
{
	socket()->writeFrameData(frame_data);
}

void SocketRpcConnection::sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	shv::chainpack::RpcDriver::sendRpcMessage(rpc_msg);
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
