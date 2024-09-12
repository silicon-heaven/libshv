#pragma once

#include "socket.h"

class QLocalSocket;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT LocalSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	enum class Protocol {Stream, Serial};
	LocalSocket(QLocalSocket *socket, Protocol protocol, QObject *parent = nullptr);
	~LocalSocket() override;

	std::vector<chainpack::RpcFrame> takeFrames() override;
	void writeFrameData(const std::string &frame_data) override;

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;

	void resetCommunication() override;

	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	void ignoreSslErrors() override;
protected:
	void onDataReadyRead();
	void flushWriteBuffer();
protected:
	QLocalSocket *m_socket = nullptr;
	FrameReader *m_frameReader;
	FrameWriter *m_frameWriter;
};

} // namespace shv::iotqt::rpc
