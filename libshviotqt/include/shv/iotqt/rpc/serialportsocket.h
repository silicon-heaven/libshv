#pragma once

#include <shv/iotqt/shviotqtglobal.h>
#include <shv/iotqt/rpc/socket.h>

#include <shv/chainpack/crc32.h>
#include <shv/coreqt/log.h>

class QSerialPort;
class QTimer;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT SerialFrameReader : public FrameReader
{
public:
	enum class ReadState {WaitingForStx, WaitingForEtx, WaitingForCrc};
	enum class CrcCheck {No, Yes};
public:
	SerialFrameReader(CrcCheck crc);
	~SerialFrameReader() override = default;

	QList<int> addData(std::string_view data) override;
	ReadState readState() const { return m_readState; }
private:
	bool inEscape() const;
	void setState(ReadState state);
	void finishFrame();
private:
	ReadState m_readState = ReadState::WaitingForStx;
	uint8_t m_recentByte = 0;
	std::string m_readBuffer;
	std::string m_crcBuffer;
	shv::chainpack::Crc32Shv3 m_crcDigest;
	bool m_withCrcCheck = true;
};

class SHVIOTQT_DECL_EXPORT SerialFrameWriter : public FrameWriter
{
public:
	enum class CrcCheck {No, Yes};
public:
	SerialFrameWriter(CrcCheck crc);
	~SerialFrameWriter() override = default;

	void addFrame(const std::string &frame_data) override;
	void resetCommunication() override;
private:
	bool m_withCrcCheck = true;
};

class SHVIOTQT_DECL_EXPORT SerialPortSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	SerialPortSocket(QSerialPort *port, QObject *parent = nullptr);

	void setReceiveTimeout(int millis);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
protected:
	void restartReceiveTimeoutTimer();
private:
	void setState(QAbstractSocket::SocketState state);
	void onDataReadyRead();
	void flushWriteBuffer() override;
	void onParseDataException(const shv::chainpack::ParseException &e) override;
	qint64 writeBytesEscaped(const char *data, qint64 max_size);
	void clearWriteBuffer() override;
private:
	QSerialPort *m_port = nullptr;
	QAbstractSocket::SocketState m_state = QAbstractSocket::UnconnectedState;
	QTimer *m_readDataTimeout = nullptr;
};
}
