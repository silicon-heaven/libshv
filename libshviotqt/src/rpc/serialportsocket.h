#pragma once

#include "../shviotqtglobal.h"
#include "socket.h"

#include <shv/chainpack/crc32.h>
#include <shv/coreqt/log.h>

#include <QQueue>

class QSerialPort;
class QTimer;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT SerialPortSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	enum class ReadMessageError {Ok = 0, ErrorUnexpectedStx, ErrorUnexpectedEtx, ErrorEscape, ErrorCrc, ErrorTimeout};
	enum EscCodes {
		STX = 0xA2,
		ETX = 0xA3,
		ESTX = 0xA4,
		EETX = 0xA5,
		ESC = 0xAA,
	};
public:
	SerialPortSocket(QSerialPort *port, QObject *parent = nullptr);

	void setReceiveTimeout(int millis);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	void reset();
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QString readMessageErrorString() const;
	ReadMessageError readMessageError() const;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	void writeMessageBegin() override;
	void writeMessageEnd() override;
	void ignoreSslErrors() override;
protected:
	void restartReceiveTimeoutTimer();
private:

	class UnescapeBuffer
	{
	public:
		ReadMessageError append(uint8_t b);
		void clear();

		QByteArray data;
		bool inEscape = false;
	};

	enum class ReadMessageState {WaitingForStx, WaitingForEtx, WaitingForCrc};

	void setState(QAbstractSocket::SocketState state);
	void onSerialDataReadyRead();
	void onParseDataException(const shv::chainpack::ParseException &e) override;
	void setReadMessageState(ReadMessageState st);
	void setReadMessageError(ReadMessageError err);
	qint64 writeBytesEscaped(const char *data, qint64 max_size);
private:
	QSerialPort *m_port = nullptr;
	QAbstractSocket::SocketState m_state = QAbstractSocket::UnconnectedState;
	ReadMessageState m_readMessageState = ReadMessageState::WaitingForStx;
	ReadMessageError m_readMessageError = ReadMessageError::Ok;
	QTimer *m_readDataTimeout = nullptr;

	shv::chainpack::Crc32Posix m_readMessageCrc;
	UnescapeBuffer m_readMessageBuffer;
	UnescapeBuffer m_readMessageCrcBuffer;
	QQueue<QByteArray> m_receivedMessages;

	bool m_escWritten = false;
	shv::chainpack::Crc32Posix m_writeMessageCrc;
};
}
