#pragma once

#include "../shviotqtglobal.h"
#include "socket.h"

#include <shv/coreqt/log.h>

#include <QObject>
#include <QAbstractSocket>
#include <QSslSocket>
#include <QSslError>

class QSerialPort;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT SerialPortSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	SerialPortSocket(QSerialPort *port, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override { return m_state; }
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	//bool flush() override;
	void writeMessageBegin() override {}
	void writeMessageEnd() override;
	void ignoreSslErrors() override {}
private:
	void setState(QAbstractSocket::SocketState state);
	void onParseDataException(const shv::chainpack::ParseException &e) override;
private:
	QSerialPort *m_port = nullptr;
	QAbstractSocket::SocketState m_state = QAbstractSocket::UnconnectedState;
};
}
