#include <shv/iotqt/rpc/serialportsocket.h>

#include <shv/chainpack/rpc.h>
#include <shv/chainpack/utils.h>

#include <QHostAddress>
#include <QSerialPort>
#include <QUrl>
#include <QTimer>

#define logSerialPortSocketD() nCDebug("SerialPortSocket")
#define logSerialPortSocketM() nCMessage("SerialPortSocket")
#define logSerialPortSocketW() nCWarning("SerialPortSocket")

#define logRpcData() nCMessage("RpcData")

using namespace std;

namespace shv::iotqt::rpc {

//======================================================
// SerialFrameReader
//======================================================
enum EscCodes: uint8_t {
	STX = 0xA2,
	ETX = 0xA3,
	ATX = 0xA4,
	ESC = 0xAA,
	ESTX = 0x02,
	EETX = 0x03,
	EATX = 0x04,
	EESC = 0x0A,
};

SerialFrameReader::SerialFrameReader(CrcCheck crc)
	: m_withCrcCheck(crc == CrcCheck::Yes)
{

}

QList<int> SerialFrameReader::addData(std::string_view data)
{
	logRpcData().nospace() << "FRAME DATA READ " << data.size() << " bytes of data read:\n" << shv::chainpack::utils::hexDump(data);
	QList<int> response_request_ids;
	auto check_response_id = [this, &response_request_ids]() {
		std::istringstream in(m_readBuffer);
		if (auto rqid = tryToReadMeta(in); rqid > 0) {
			response_request_ids << rqid;
		}
	};
	auto add_byte = [this](string &buff, uint8_t b) {
		if (m_recentByte == ESC) {
			switch (b) {
			case ESTX: buff += static_cast<char>(STX); break;
			case EETX: buff += static_cast<char>(ETX); break;
			case EATX: buff += static_cast<char>(ATX); break;
			case EESC: buff += static_cast<char>(ESC); break;
			default: {
				shvWarning() << "Invalid escape sequence:" << m_recentByte << b;
				setState(ReadState::WaitingForStx);
				break;
			}
			}
		}
		else {
			if (b != ESC) {
				buff += static_cast<char>(b);
			}
		}
	};
	for (uint8_t b : data) {
		if (b == STX) {
			setState(ReadState::WaitingForEtx);
			m_recentByte = b;
			continue;
		}
		if (b == ATX) {
			setState(ReadState::WaitingForStx);
			m_recentByte = b;
			continue;
		}
		switch (m_readState) {
		case ReadState::WaitingForStx: {
			if (b == STX) {
				setState(ReadState::WaitingForEtx);
			}
			m_recentByte = b;
			continue;
		}
		case ReadState::WaitingForEtx: {
			if (b == ETX) {
				if (m_withCrcCheck) {
					setState(ReadState::WaitingForCrc);
				}
				else {
					check_response_id();
					finishFrame();
				}
				continue;
			}
			m_crcDigest.add(b);
			add_byte(m_readBuffer, b);
			m_recentByte = b;
			continue;
		}
		case ReadState::WaitingForCrc: {
			add_byte(m_crcBuffer, b);
			m_recentByte = b;
			if (m_crcBuffer.size() == 4) {
				check_response_id();
				finishFrame();
			}
			continue;
		}
		}
	}
	check_response_id();
	return response_request_ids;
}

void SerialFrameReader::resetCommunication()
{
	m_readState = ReadState::WaitingForStx;
	m_recentByte = 0;
	m_readBuffer.clear();
	m_crcBuffer.clear();
	m_crcDigest = {};
	Super::resetCommunication();
}

void SerialFrameReader::setState(ReadState state)
{
	m_readState = state;
	switch (state) {
	case ReadState::WaitingForStx: {
		break;
	}
	case ReadState::WaitingForEtx: {
		m_readBuffer = {};
		m_meta = {};
		m_dataStart = {};
		m_crcDigest.reset();
		break;
	}
	case ReadState::WaitingForCrc: {
		m_crcBuffer = {};
		break;
	}
	}
}

void SerialFrameReader::finishFrame()
{
	if (m_withCrcCheck) {
		Q_ASSERT(m_crcBuffer.size() == 4);
		shv::chainpack::crc32_t msg_crc = 0;
		for(uint8_t bb : m_crcBuffer) {
			msg_crc <<= 8;
			msg_crc += bb;
		}
		logSerialPortSocketD() << "crc data:" << QByteArray(m_crcBuffer.data(), m_crcBuffer.size()).toHex().toStdString();
		logSerialPortSocketD() << "crc received:" << shv::chainpack::utils::intToHex(msg_crc);
		logSerialPortSocketD() << "crc computed:" << shv::chainpack::utils::intToHex(m_crcDigest.result());
		if(m_crcDigest.result() != msg_crc) {
			auto err = QStringLiteral("CRC error, expected: %1, got: %2")
					.arg(m_crcDigest.result(), 4, 16, QChar('0'))
					.arg(msg_crc, 4, 16, QChar('0'));
			shvWarning() << err;
			setState(ReadState::WaitingForStx);
			return;
		}
	}
	if (!m_dataStart.has_value()) {
		logSerialPortSocketD() << "Protocol type or read meta Error";
		setState(ReadState::WaitingForStx);
		return;
	}
	shvDebug() << "ADD FRAME:" << chainpack::utils::hexArray(m_readBuffer.data(), m_readBuffer.size());
	auto frame_data = std::string(m_readBuffer, m_dataStart.value());
	m_frames.emplace_back(m_protocol, std::move(m_meta), std::move(frame_data));
	setState(ReadState::WaitingForStx);
}

//======================================================
// SerialFrameWriter
//======================================================
SerialFrameWriter::SerialFrameWriter(CrcCheck crc)
	: m_withCrcCheck(crc == CrcCheck::Yes)
{
}

void SerialFrameWriter::addFrameData(const std::string &frame_head, const std::string &frame_data)
{
	QByteArray data_to_write;
	auto write_escaped = [&data_to_write](uint8_t b) {
		switch (b) {
		case STX: data_to_write += static_cast<char>(ESC); data_to_write += static_cast<char>(ESTX); break;
		case ETX: data_to_write += static_cast<char>(ESC); data_to_write += static_cast<char>(EETX); break;
		case ATX: data_to_write += static_cast<char>(ESC); data_to_write += static_cast<char>(EATX); break;
		case ESC: data_to_write += static_cast<char>(ESC); data_to_write += static_cast<char>(EESC); break;
		default: data_to_write += static_cast<char>(b); break;
		}
	};
	data_to_write += static_cast<char>(STX);
	for(uint8_t b : frame_head) {
		write_escaped(b);
	}
	for(uint8_t b : frame_data) {
		write_escaped(b);
	}
	data_to_write += static_cast<char>(ETX);
	if (m_withCrcCheck) {
		shv::chainpack::Crc32Shv3 crc_digest;
		crc_digest.add(data_to_write.constData() + 1, data_to_write.size() - 2);
		auto crc = crc_digest.result();
		for (int i = 0; i < 4; ++i) {
			write_escaped((crc >> ((3 - i) * 8)) & 0xff);
		}
	}
	m_messageDataToWrite << data_to_write;
}

void SerialFrameWriter::resetCommunication()
{
	NecroLog::create(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, shv::chainpack::Rpc::TOPIC_RPC_MSG))
		<< shv::chainpack::Rpc::SND_LOG_ARROW
		<< "<RESET_SESSION>";

	QByteArray data_to_write;
	data_to_write.append(static_cast<char>(00));
	addFrameData(data_to_write.toStdString(), {});
}

//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(std::make_unique<SerialFrameReader>(SerialFrameReader::CrcCheck::Yes), std::make_unique<SerialFrameWriter>(SerialFrameWriter::CrcCheck::Yes), parent)
	, m_port(port)
{
	m_port->setParent(this);

	connect(m_port, &QSerialPort::readyRead, this, &SerialPortSocket::onDataReadyRead);
	connect(m_port, &QSerialPort::bytesWritten, this, &SerialPortSocket::flushWriteBuffer);
	connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError port_error) {
		switch (port_error) {
		case QSerialPort::NoError:
			break;
		case QSerialPort::DeviceNotFoundError:
			shvDebug() << "DeviceNotFoundError";
			emit error(QAbstractSocket::HostNotFoundError);
			break;
		case QSerialPort::PermissionError:
			shvDebug() << "PermissionError";
			emit error(QAbstractSocket::SocketAccessError);
			break;
		case QSerialPort::OpenError:
			shvDebug() << "OpenError";
			emit error(QAbstractSocket::ConnectionRefusedError);
			break;
#if QT_VERSION_MAJOR < 6
		case QSerialPort::ParityError:
		case QSerialPort::FramingError:
		case QSerialPort::BreakConditionError:
#endif
		case QSerialPort::WriteError:
			shvDebug() << "WriteError";
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::ReadError:
			shvDebug() << "ReadError";
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::ResourceError:
			shvDebug() << "ResourceError";
			emit error(QAbstractSocket::SocketResourceError);
			break;
		case QSerialPort::UnsupportedOperationError:
			shvDebug() << "UnsupportedOperationError";
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::UnknownError:
			shvDebug() << "UnknownError";
			emit error(QAbstractSocket::UnknownSocketError);
			break;
		case QSerialPort::TimeoutError:
			shvDebug() << "TimeoutError";
			emit error(QAbstractSocket::SocketTimeoutError);
			break;
		case QSerialPort::NotOpenError:
			shvDebug() << "NotOpenError";
			emit error(QAbstractSocket::SocketAccessError);
			break;
		}
	});

	setReceiveTimeout(5000);
}

void SerialPortSocket::setReceiveTimeout(int millis)
{
	if(millis <= 0) {
		if(m_readDataTimeout) {
			delete m_readDataTimeout;
			m_readDataTimeout = nullptr;
		}
	}
	else {
		if(!m_readDataTimeout) {
			m_readDataTimeout = new QTimer(this);
			m_readDataTimeout->setSingleShot(true);
			connect(m_readDataTimeout, &QTimer::timeout, this, [this]() {
				auto &frame_reader = dynamic_cast<SerialFrameReader&>(frameReader());
				if(frame_reader.readState() != SerialFrameReader::ReadState::WaitingForStx) {
					resetCommunication();
				}
			});
		}
		m_readDataTimeout->setInterval(millis);
	}
}

void SerialPortSocket::connectToHost(const QUrl &url)
{
	abort();
	setState(QAbstractSocket::ConnectingState);
	m_port->setPortName(url.path());
	shvInfo() << "opening serial port:" << m_port->portName();
	if(m_port->open(QIODevice::ReadWrite)) {
		setState(QAbstractSocket::ConnectedState);
	}
	else {
		shvWarning() << "Error open serial port:" << m_port->portName() << m_port->errorString();
		setState(QAbstractSocket::UnconnectedState);
	}
}

void SerialPortSocket::close()
{
	if(state() == QAbstractSocket::UnconnectedState)
		return;
	setState(QAbstractSocket::ClosingState);
	Super::close();
	shvInfo() << "Closing serial port:" << m_port->portName();
	if (m_port->isOpen()) {
		m_port->close();
	}
	setState(QAbstractSocket::UnconnectedState);
}

void SerialPortSocket::abort()
{
	close();
}

QAbstractSocket::SocketState SerialPortSocket::state() const
{
	return m_state;
}

QString SerialPortSocket::errorString() const
{
	return m_port->errorString();
}

QHostAddress SerialPortSocket::peerAddress() const
{
	return QHostAddress(m_port->portName());
}

quint16 SerialPortSocket::peerPort() const
{
	return 0;
}

void SerialPortSocket::onDataReadyRead()
{
	auto ba = m_port->readAll();
	string_view escaped_data(ba.constData(), ba.size());
	for (auto rqid : frameReader().addData(escaped_data)) {
		emit responseMetaReceived(rqid);
	}
	emit dataChunkReceived();
	if (!frameReader().isEmpty()) {
		emit readyRead();
	}
}

void SerialPortSocket::flushWriteBuffer()
{
	frameWriter().flushToDevice(m_port);
	m_port->flush();
}

void SerialPortSocket::ignoreSslErrors()
{
}

void SerialPortSocket::restartReceiveTimeoutTimer()
{
	// m_readDataTimeout is set to nullptr during unit tests
	if(m_readDataTimeout)
		m_readDataTimeout->start();
}

void SerialPortSocket::setState(QAbstractSocket::SocketState state)
{
	if(state == m_state)
		return;
	m_state = state;
	emit stateChanged(m_state);
	if(m_state == QAbstractSocket::SocketState::ConnectedState)
		emit connected();
	else if(m_state == QAbstractSocket::SocketState::UnconnectedState)
		emit disconnected();
}

void SerialPortSocket::onParseDataException(const chainpack::ParseException &)
{
	// nothing to do
}

void SerialPortSocket::clearWriteBuffer()
{
	Super::clearWriteBuffer();
	if (m_port->isOpen()) {
		m_port->clear();
	}
}

}
