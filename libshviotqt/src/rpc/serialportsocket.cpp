#include <shv/iotqt/rpc/serialportsocket.h>

#include <shv/chainpack/utils.h>

#include <QHostAddress>
#include <QSerialPort>
#include <QUrl>
#include <QTimer>

#define logSerialPortSocketD() nCDebug("SerialPortSocket")
#define logSerialPortSocketM() nCMessage("SerialPortSocket")
#define logSerialPortSocketW() nCWarning("SerialPortSocket")

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

void SerialFrameReader::addData(std::string_view data)
{
	auto add_byte = [this](string &buff, uint8_t b) {
		m_crcDigest.add(b);
		if (inEscape()) {
			switch (b) {
			case ESTX: buff += static_cast<char>(STX); break;
			case EETX: buff += static_cast<char>(ETX); break;
			case EATX: buff += static_cast<char>(ATX); break;
			case EESC: buff += static_cast<char>(ESC); break;
			default: throw std::runtime_error("Invalid escap sequention");
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
					finishFrame();
				}
				continue;
			}
			add_byte(m_readBuffer, b);
			m_recentByte = b;
			continue;
		}
		case ReadState::WaitingForCrc: {
			add_byte(m_crcBuffer, b);
			m_recentByte = b;
			if (m_crcBuffer.size() == 4) {
				finishFrame();
			}
			continue;
		}
		}
	}
}

bool SerialFrameReader::inEscape() const
{
	return m_recentByte == ESC;
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
		//logSerialPortSocketD() << "crc data:" << m_crcBuffer.toHex().toStdString();
		logSerialPortSocketD() << "crc received:" << shv::chainpack::utils::intToHex(msg_crc);
		logSerialPortSocketD() << "crc computed:" << shv::chainpack::utils::intToHex(m_crcDigest.remainder());
		if(m_crcDigest.remainder() != msg_crc) {
			logSerialPortSocketD() << "crc OK";
		}
	}
	m_frames.push_back(std::move(m_readBuffer));
	setState(ReadState::WaitingForStx);
}

//======================================================
// SerialFrameWriter
//======================================================
void SerialFrameWriter::addFrame(std::string &&frame_data)
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
	for(uint8_t b : frame_data) {
		write_escaped(b);
	}
	data_to_write += static_cast<char>(ETX);
	if (m_withCrcCheck) {
		shv::chainpack::Crc32Posix crc_digest;
		crc_digest.add(frame_data.data(), frame_data.size());
		auto crc = crc_digest.remainder();
		for (int i = 0; i < 4; ++i) {
			write_escaped((crc >> (3 - i)) & 0xff);
		}
	}
	m_messagesToWrite << data_to_write;
}

//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(parent)
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
			emit error(QAbstractSocket::HostNotFoundError);
			break;
		case QSerialPort::PermissionError:
			emit error(QAbstractSocket::SocketAccessError);
			break;
		case QSerialPort::OpenError:
			emit error(QAbstractSocket::ConnectionRefusedError);
			break;
#if QT_VERSION_MAJOR < 6
		case QSerialPort::ParityError:
		case QSerialPort::FramingError:
		case QSerialPort::BreakConditionError:
#endif
		case QSerialPort::WriteError:
		case QSerialPort::ReadError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::ResourceError:
			emit error(QAbstractSocket::SocketResourceError);
			break;
		case QSerialPort::UnsupportedOperationError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::UnknownError:
			emit error(QAbstractSocket::UnknownSocketError);
			break;
		case QSerialPort::TimeoutError:
			emit error(QAbstractSocket::SocketTimeoutError);
			break;
		case QSerialPort::NotOpenError:
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
				//if(m_readMessageState != ReadMessageState::WaitingForStx) {
				//	setReadMessageError(ReadMessageError::ErrorTimeout);
				//}
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
		shvInfo() << "Ok";
		reset();
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
	m_port->close();
	setState(QAbstractSocket::UnconnectedState);
}

void SerialPortSocket::abort()
{
	close();
}

void SerialPortSocket::reset()
{
	writeFrameData({});
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

std::string SerialPortSocket::readFrameData()
{
	if(m_frameReader.isEmpty())
		return {};
	return m_frameReader.getFrame();
}

void SerialPortSocket::writeFrameData(std::string &&frame_data)
{
	m_frameWriter.addFrame(std::move(frame_data));
	flushWriteBuffer();
}

void SerialPortSocket::onDataReadyRead()
{
	auto ba = m_port->readAll();
	string_view escaped_data(ba.constData(), ba.size());
	m_frameReader.addData(escaped_data);
	if (!m_frameReader.isEmpty()) {
		emit readyRead();
	}
}

void SerialPortSocket::flushWriteBuffer()
{
	while (true) {
		auto data = m_frameWriter.getMessageDataToWrite();
		if (data.isEmpty())
			break;
		auto n = m_port->write(data);
		if (n > 0 && n < data.size()) {
			data = data.mid(n);
			m_frameWriter.pushUnwrittenMessageData(data);
		}
	}
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

}
