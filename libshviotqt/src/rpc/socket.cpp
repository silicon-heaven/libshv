#include <shv/iotqt/rpc/socket.h>

#include <shv/coreqt/log.h>
#include <shv/chainpack/utils.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/cponreader.h>
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

#define logRpcData() nCMessage("RpcData")

namespace shv::iotqt::rpc {

//======================================================
// FrameWriter
//======================================================
void FrameWriter::addFrame(const chainpack::RpcFrame &frame)
{
	try {
		auto frame_head = frame.toFrameHead();
		addFrameData(frame_head, frame.data);
	} catch (const std::runtime_error &e) {
		shvWarning() << "Error converting frame to data:" << e.what();
	}
}

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
		static constexpr auto protocol_cpon = static_cast<int>(shv::chainpack::Rpc::ProtocolType::Cpon);
		auto protocol = in.get();
		std::unique_ptr<AbstractStreamReader> rd;
		if (protocol == protocol_cpon) {
			m_protocol = shv::chainpack::Rpc::ProtocolType::Cpon;
			rd = std::make_unique<chainpack::CponReader>(in);
		}
		else if (protocol == protocol_chainpack) {
			m_protocol = shv::chainpack::Rpc::ProtocolType::ChainPack;
			rd = std::make_unique<chainpack::ChainPackReader>(in);
		}
		if (rd) {
			try {
				m_meta = {};
				rd->read(m_meta);
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
	logRpcData().nospace() << "FRAME DATA READ " << data.size() << " bytes of data read:\n" << shv::chainpack::utils::hexDump(data);
	using namespace chainpack;
	QList<int> response_request_ids;
	m_readBuffer += data;
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
		auto frame_len_size = static_cast<size_t>(len);
		if (auto rqid = tryToReadMeta(in); rqid > 0) {
			response_request_ids << rqid;
		}
		if (frame_len_size + frame_size <= m_readBuffer.size()) {
			if (!m_dataStart.has_value()) {
				throw std::runtime_error("Read RPC message meta data error.");
			}
			auto data_start = m_dataStart.value();
			auto data_len = frame_len_size + frame_size - m_dataStart.value();
			auto frame_data = std::string(m_readBuffer, data_start, data_len);
			m_readBuffer = std::string(m_readBuffer, frame_len_size + frame_size);
			m_frames.emplace_back(m_protocol, std::move(m_meta), std::move(frame_data));
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
void StreamFrameWriter::addFrameData(const std::string &frame_head, const std::string &frame_data)
{
	using namespace shv::chainpack;
	std::ostringstream out;
	{
		ChainPackWriter wr(out);
		wr.writeUIntData(frame_head.size() + frame_data.size());
	}
	auto len_data = out.str();
	QByteArray data(len_data.data(), len_data.size());
	data.append(frame_head.data(), frame_head.size());
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
	delete m_frameReader;
	delete m_frameWriter;
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

void Socket::writeFrame(const shv::chainpack::RpcFrame &frame)
{
	Q_ASSERT(m_frameWriter);
	m_frameWriter->addFrame(frame);
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
	try {
		std::string_view data(ba.constData(), ba.size());
		for (auto rqid : m_frameReader->addData(data)) {
			emit responseMetaReceived(rqid);
		}
		emit dataChunkReceived();
		if (!m_frameReader->isEmpty()) {
			emit readyRead();
		}
	}
	catch (const std::runtime_error &e) {
		shvWarning() << "Corrupted meta data received:" << e.what() << "\n" << shv::chainpack::utils::hexDump(std::string_view(ba.constData(), std::min(ba.size(), static_cast<decltype(ba.size())>(64))));
		emit error(QAbstractSocket::SocketError::UnknownSocketError);
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
