#pragma once

#include "../shviotqtglobal.h"

#include <QObject>
#include <QAbstractSocket>
#include <QSslSocket>
#include <QSslError>

class QTcpSocket;
class QLocalSocket;
class QSerialPort;

namespace shv::chainpack { class ParseException; }

namespace shv {
namespace iotqt {
namespace rpc {

/// wrapper class for QTcpSocket and QWebSocket

class SHVIOTQT_DECL_EXPORT Socket : public QObject
{
	Q_OBJECT
public:
	enum class Scheme { Tcp = 0, Ssl, WebSocket, WebSocketSecure, SerialPort, LocalSocket };
public:
	explicit Socket(QObject *parent = nullptr);

	static const char* schemeToString(Scheme schema);
	static Scheme schemeFromString(const std::string &schema);

	virtual void connectToHost(const QUrl &host_url) = 0;

	virtual void close() = 0;
	virtual void abort() = 0;

	virtual QAbstractSocket::SocketState state() const = 0;
	virtual QString errorString() const = 0;

	virtual QHostAddress  peerAddress() const = 0;
	virtual quint16  peerPort() const = 0;

	virtual QByteArray readAll() = 0;
	virtual qint64 write(const char *data, qint64 max_size) = 0;
	//virtual bool flush() = 0;
	virtual void writeMessageBegin() = 0;
	virtual void writeMessageEnd() = 0;
	virtual void ignoreSslErrors() = 0;

	virtual void onParseDataException(const shv::chainpack::ParseException &) { abort(); }

	Q_SIGNAL void connected();
	Q_SIGNAL void disconnected();
	Q_SIGNAL void readyRead();
	//Q_SIGNAL void readyWrite();
	Q_SIGNAL void  bytesWritten(qint64 bytes);

	Q_SIGNAL void  stateChanged(QAbstractSocket::SocketState state);
	Q_SIGNAL void error(QAbstractSocket::SocketError socket_error);
	Q_SIGNAL void sslErrors(const QList<QSslError> &errors);
};

class SHVIOTQT_DECL_EXPORT TcpSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	TcpSocket(QTcpSocket *socket, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	//bool flush() override;
	void writeMessageBegin() override {}
	void writeMessageEnd() override;
	void ignoreSslErrors() override {}

protected:
	QTcpSocket *m_socket = nullptr;
};

class SHVIOTQT_DECL_EXPORT LocalSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	LocalSocket(QLocalSocket *socket, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	//bool flush() override;
	void writeMessageBegin() override {}
	void writeMessageEnd() override {}
	void ignoreSslErrors() override {}
protected:
	QLocalSocket *m_socket = nullptr;
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
} // namespace rpc
} // namespace iotqt
} // namespace shv
