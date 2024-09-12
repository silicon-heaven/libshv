#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcmessage.h>

#include <QObject>
#include <QAbstractSocket>
#include <QSslSocket>
#include <QSslError>

#include <vector>

class QTcpSocket;

#ifdef WITH_SHV_WEBSOCKETS
class QWebSocket;
#endif

namespace shv::chainpack { class ParseException; }

namespace shv::iotqt::rpc {

class FrameReader {
public:
	virtual ~FrameReader() = default;
	virtual QList<int> addData(std::string_view data) = 0;
	bool isEmpty() const { return m_frames.empty(); }
	std::vector<chainpack::RpcFrame> takeFrames() {
		auto frames = std::move(m_frames);
		m_frames = {};
		return frames;
	}
protected:
	int tryToReadMeta(std::istringstream &in);
protected:
	std::vector<chainpack::RpcFrame> m_frames;
	chainpack::RpcValue::MetaData m_meta;
	std::optional<size_t> m_dataStart;

};

class FrameWriter
{
public:
	virtual ~FrameWriter() = default;
	virtual void addFrame(const std::string &frame_data) = 0;
	virtual void resetCommunication() {}
	void flushToDevice(QIODevice *device);
#ifdef WITH_SHV_WEBSOCKETS
	void flushToWebSocket(QWebSocket *socket);
#endif
protected:
	QList<QByteArray> m_messageDataToWrite;
};

class StreamFrameReader : public FrameReader
{
public:
	~StreamFrameReader() override = default;

	QList<int> addData(std::string_view data) override;
private:
	std::string m_readBuffer;
};

class StreamFrameWriter : public FrameWriter
{
public:
	~StreamFrameWriter() override = default;

	void addFrame(const std::string &frame_data) override;
};

/// wrapper class for QTcpSocket and QWebSocket

class SHVIOTQT_DECL_EXPORT Socket : public QObject
{
	Q_OBJECT
public:
	enum class Scheme { Tcp = 0, Ssl, WebSocket, WebSocketSecure, SerialPort, LocalSocket, LocalSocketSerial };
public:
	explicit Socket(QObject *parent = nullptr);

	static const char* schemeToString(Scheme schema);
	static Scheme schemeFromString(const std::string &schema);

	virtual void connectToHost(const QUrl &host_url) = 0;

	virtual void close() = 0;
	virtual void abort() = 0;
	virtual void resetCommunication() {}

	virtual QAbstractSocket::SocketState state() const = 0;
	virtual QString errorString() const = 0;

	virtual QHostAddress peerAddress() const = 0;
	virtual quint16 peerPort() const = 0;

	virtual std::vector<chainpack::RpcFrame> takeFrames() = 0;
	virtual void writeFrameData(const std::string &frame_data) = 0;

	virtual void ignoreSslErrors() = 0;

	virtual void onParseDataException(const shv::chainpack::ParseException &);

	Q_SIGNAL void connected();
	Q_SIGNAL void disconnected();
	Q_SIGNAL void readyRead();
	Q_SIGNAL void responseMetaReceived(int request_id);
	Q_SIGNAL void dataChunkReceived();

	Q_SIGNAL void stateChanged(QAbstractSocket::SocketState state);
	Q_SIGNAL void error(QAbstractSocket::SocketError socket_error);
	Q_SIGNAL void sslErrors(const QList<QSslError> &errors);
};

class SHVIOTQT_DECL_EXPORT TcpSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	TcpSocket(QTcpSocket *socket, QObject *parent = nullptr);

	std::vector<chainpack::RpcFrame> takeFrames() override;
	void writeFrameData(const std::string &frame_data) override;

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
protected:
	void onDataReadyRead();
	void flushWriteBuffer();
protected:
	QTcpSocket *m_socket = nullptr;
	StreamFrameReader m_frameReader;
	StreamFrameWriter m_frameWriter;
};

#ifndef QT_NO_SSL
class SHVIOTQT_DECL_EXPORT SslSocket : public TcpSocket
{
	Q_OBJECT

	using Super = TcpSocket;
public:
	SslSocket(QSslSocket *socket, QSslSocket::PeerVerifyMode peer_verify_mode = QSslSocket::AutoVerifyPeer, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void ignoreSslErrors() override;

protected:
	QSslSocket::PeerVerifyMode m_peerVerifyMode;
};
#endif
} // namespace shv::iotqt::rpc


