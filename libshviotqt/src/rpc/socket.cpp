#include <shv/iotqt/rpc/socket.h>

#include <shv/coreqt/log.h>
#include <shv/chainpack/utils.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/irpcconnection.h>

#include <QHostAddress>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#ifdef WITH_SHV_WEBSOCKETS
#include <QWebSocket>
#endif

namespace shv::iotqt::rpc {

//======================================================
// FrameWriter
//======================================================
void FrameWriter::flushToDevice(QIODevice *device)
{
	while (!m_messageDataToWrite.isEmpty()) {
		auto data = m_messageDataToWrite[0];
		auto n = device->write(data);
		shvDebug() << "<=== sending:" << data.size() << "bytes:" << chainpack::utils::hexArray(data.constData(), data.size());
		if (n <= 0) {
			shvWarning() << "Write data error.";
			break;
		}
		if (n < data.size()) {
			data = data.mid(static_cast<qsizetype>(n));
			m_messageDataToWrite[0] = data;
		}
		else {
			m_messageDataToWrite.takeFirst();
		}
	}
}

void FrameWriter::clear()
{
	m_messageDataToWrite.clear();
}

#ifdef WITH_SHV_WEBSOCKETS
void FrameWriter::flushToWebSocket(QWebSocket *socket)
{
	while (!m_messageDataToWrite.isEmpty()) {
		auto data = m_messageDataToWrite[0];
		auto n = socket->sendBinaryMessage(data);
		if (n != data.size()) {
			shvWarning() << "Write data error.";
			break;
		}
		m_messageDataToWrite.takeFirst();
	}
}
#endif


//======================================================
// FrameReader
//======================================================
int FrameReader::tryToReadMeta(std::istringstream &in)
{
	if (!m_dataStart.has_value()) {
		using namespace chainpack;
		static constexpr auto protocol_chainpack = static_cast<int>(shv::chainpack::Rpc::ProtocolType::ChainPack);
		if (auto b = in.get(); b == protocol_chainpack) {
			chainpack::ChainPackReader rd(in);
			try {
				m_meta = {};
				rd.read(m_meta);
				auto data_start = in.tellg();
				if (data_start == -1) {
					// meta was read without error, but without closing brackets
					return 0;
				}
				m_dataStart = static_cast<size_t>(data_start);
				if (chainpack::RpcMessage::isResponse(m_meta)) {
					if (auto rqid = chainpack::RpcMessage::requestId(m_meta).toInt(); rqid > 0) {
						return rqid;
					}
				}
				return 0;
			}
			catch (ParseException &e) {
				// ignore read errro, meta not availble or corrupted
				// frame parser will catch it later on
			}
		}
	}
	return 0;
}

//======================================================
// StreamFrameReader
//======================================================
QList<int> StreamFrameReader::addData(std::string_view data)
{
	using namespace chainpack;
	QList<int> response_request_ids;
	m_readBuffer += data;
	shvDebug() << "received:\n" << utils::hexDump(data);
	while (true) {
		std::istringstream in(m_readBuffer);
		int err_code;
		auto frame_size = static_cast<size_t>(ChainPackReader::readUIntData(in, &err_code));
		if(err_code == CCPCP_RC_BUFFER_UNDERFLOW) {
			// not enough data
			break;
		}
		if(err_code != CCPCP_RC_OK) {
			throw std::runtime_error("Read RPC message length error.");
		}
		auto len = in.tellg();
		if (len <= 0) {
			throw std::runtime_error("Read RPC message length data error.");
		}
		auto consumed_len = static_cast<size_t>(len);
		if (auto rqid = tryToReadMeta(in); rqid > 0) {
			response_request_ids << rqid;
		}
		if (consumed_len + frame_size <= m_readBuffer.size()) {
			if (!m_dataStart.has_value()) {
				throw std::runtime_error("Read RPC message meta data error.");
			}
			auto frame_data = std::string(m_readBuffer, m_dataStart.value(), frame_size);
			m_readBuffer = std::string(m_readBuffer, consumed_len + frame_size);
			m_frames.emplace_back(std::move(m_meta), std::move(frame_data));
			m_meta = {};
			m_dataStart = {};
		}
		else {
			break;
		}
	}
	return response_request_ids;
}

//======================================================
// StreamFrameWriter
//======================================================
void StreamFrameWriter::addFrame(const std::string &frame_data)
{
	using namespace shv::chainpack;
	std::ostringstream out;
	{
		ChainPackWriter wr(out);
		wr.writeUIntData(frame_data.size());
	}
	auto len_data = out.str();
	QByteArray data(len_data.data(), len_data.size());
	data.append(frame_data.data(), frame_data.size());
	m_messageDataToWrite.append(data);
}

//======================================================
// Socket
//======================================================
Socket::Socket(QObject *parent)
	: QObject(parent)
{

}

Socket::~Socket()
{
	if (m_frameReader) {
		delete m_frameReader;
	}
	if (m_frameWriter) {
		delete m_frameWriter;
	}
}

const char * Socket::schemeToString(Scheme schema)
{
	switch (schema) {
	case Scheme::Tcp: return "tcp";
	case Scheme::Ssl: return "ssl";
	case Scheme::WebSocket: return "ws";
	case Scheme::WebSocketSecure: return "wss";
	case Scheme::SerialPort: return "serial";
	case Scheme::LocalSocket: return "unix";
	case Scheme::LocalSocketSerial: return "unixs";
	}
	return "";
}

Socket::Scheme Socket::schemeFromString(const std::string &schema)
{
	if(schema == "tcp") return Scheme::Tcp;
	if(schema == "ssl") return Scheme::Ssl;
	if(schema == "ws") return Scheme::WebSocket;
	if(schema == "wss") return Scheme::WebSocketSecure;
	if(schema == "serialport" || schema == "serial") return Scheme::SerialPort;
	if(schema == "localsocket" || schema == "unix") return Scheme::LocalSocket;
	if(schema == "unixs") return Scheme::LocalSocketSerial;
	return Scheme::Tcp;
}

void Socket::close()
{
	clearWriteBuffer();
}

void Socket::abort()
{
	clearWriteBuffer();
}

bool Socket::isOpen() const
{
	return state() == QAbstractSocket::ConnectedState;
}

void Socket::resetCommunication()
{
	Q_ASSERT(m_frameWriter);
	if (isOpen()) {
		m_frameWriter->resetCommunication();
		flushWriteBuffer();
	}
}

std::vector<chainpack::RpcFrame> Socket::takeFrames()
{
	Q_ASSERT(m_frameReader);
	return m_frameReader->takeFrames();
}

void Socket::writeFrameData(const std::string &frame_data)
{
	Q_ASSERT(m_frameWriter);
	m_frameWriter->addFrame(frame_data);
	flushWriteBuffer();
}

void Socket::onParseDataException(const shv::chainpack::ParseException& ex)
{
	shvWarning() << "Frame ParseException" << ex.what();
}

void Socket::clearWriteBuffer()
{
	m_frameWriter->clear();
}

//======================================================
// TcpSocket
//======================================================
TcpSocket::TcpSocket(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	m_socket->setParent(this);
	m_frameReader = new StreamFrameReader();
	m_frameWriter = new StreamFrameWriter();

	connect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(m_socket, &QTcpSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpSocket::onDataReadyRead);
	connect(m_socket, &QTcpSocket::bytesWritten, this, &TcpSocket::flushWriteBuffer);
	connect(m_socket, &QTcpSocket::stateChanged, this, &Socket::stateChanged);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Socket::error);
#else
	connect(m_socket, &QAbstractSocket::errorOccurred, this, &Socket::error);
#endif
}

void TcpSocket::connectToHost(const QUrl &url)
{
	auto port = url.port(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED);
	m_socket->connectToHost(url.host(), static_cast<quint16>(port));
}

void TcpSocket::close()
{
	Super::close();
	m_socket->close();
}

void TcpSocket::abort()
{
	Super::abort();
	m_socket->abort();
}

QAbstractSocket::SocketState TcpSocket::state() const
{
	return m_socket->state();
}

QString TcpSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress TcpSocket::peerAddress() const
{
	return m_socket->peerAddress();
}

quint16 TcpSocket::peerPort() const
{
	return m_socket->peerPort();
}

void TcpSocket::ignoreSslErrors()
{
}

void TcpSocket::onDataReadyRead()
{
	auto ba = m_socket->readAll();
	std::string_view data(ba.constData(), ba.size());
	for (auto rqid : m_frameReader->addData(data)) {
		emit responseMetaReceived(rqid);
	}
	emit dataChunkReceived();
	if (!m_frameReader->isEmpty()) {
		emit readyRead();
	}
}

void TcpSocket::flushWriteBuffer()
{
	m_frameWriter->flushToDevice(m_socket);
	m_socket->flush();
}

//======================================================
// SslSocket
//======================================================
#ifndef QT_NO_SSL
SslSocket::SslSocket(QSslSocket *socket, QSslSocket::PeerVerifyMode peer_verify_mode, QObject *parent)
	: Super(socket, parent)
	, m_peerVerifyMode(peer_verify_mode)
{
	disconnect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(socket, &QSslSocket::encrypted, this, &Socket::connected);
	connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &Socket::sslErrors);
}

void SslSocket::connectToHost(const QUrl &url)
{
	auto *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->setPeerVerifyMode(m_peerVerifyMode);
	shvDebug() << "connectToHostEncrypted" << "host:" << url.toString();
	auto port = url.port(chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT_SECURED);
	ssl_socket->connectToHostEncrypted(url.host(), static_cast<quint16>(port));
}

void SslSocket::ignoreSslErrors()
{
	auto *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->ignoreSslErrors();
}
#endif
} // namespace shv
