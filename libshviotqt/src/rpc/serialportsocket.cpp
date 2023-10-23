#include "serialportsocket.h"

#include <shv/chainpack/utils.h>

#include <QHostAddress>
#include <QSerialPort>
#include <QUrl>
#include <QTimer>

#include <array>
#include <optional>

#define logSerialPortSocketD() nCDebug("SerialPortSocket")
#define logSerialPortSocketM() nCMessage("SerialPortSocket")
#define logSerialPortSocketW() nCWarning("SerialPortSocket")

namespace shv::iotqt::rpc {

//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(parent)
	, m_port(port)
{
	m_port->setParent(this);

	connect(m_port, &QSerialPort::readyRead, this, &SerialPortSocket::onSerialDataReadyRead);
	connect(m_port, &QSerialPort::bytesWritten, this, &Socket::bytesWritten);
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
				if(m_readMessageState != ReadMessageState::WaitingForStx) {
					setReadMessageError(ReadMessageError::ErrorTimeout);
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
	writeMessageBegin();
	writeMessageEnd();
}

QAbstractSocket::SocketState SerialPortSocket::state() const
{
	return m_state;
}

QString SerialPortSocket::errorString() const
{
	return m_port->errorString();
}

QString SerialPortSocket::readMessageErrorString() const
{
	switch(m_readMessageError) {
	case ReadMessageError::Ok: return {};
	case ReadMessageError::ErrorEscape: return QStringLiteral("Escaping error");
	case ReadMessageError::ErrorCrc: return QStringLiteral("CRC error");
	case ReadMessageError::ErrorTimeout: return QStringLiteral("Timeout error");
	case ReadMessageError::ErrorUnexpectedStx: return QStringLiteral("Unexpected STX");
	case ReadMessageError::ErrorUnexpectedEtx: return QStringLiteral("Unexpected ETX");
	}
	return {};
}

SerialPortSocket::ReadMessageError SerialPortSocket::readMessageError() const
{
	return m_readMessageError;
}

QHostAddress SerialPortSocket::peerAddress() const
{
	return QHostAddress(m_port->portName());
}

quint16 SerialPortSocket::peerPort() const
{
	return 0;
}

void SerialPortSocket::onSerialDataReadyRead()
{
	if(m_readMessageState != ReadMessageState::WaitingForStx) {
		restartReceiveTimeoutTimer();
	}
	auto escaped_data = m_port->readAll();
#if QT_VERSION_MAJOR < 6
	int ix = 0;
#else
	qsizetype ix = 0;
#endif
	logSerialPortSocketD().nospace() << "Data received:\n" << shv::chainpack::utils::hexDump(escaped_data.constData(), escaped_data.size());
	while(ix < escaped_data.size()) {
		switch(m_readMessageState) {
		case ReadMessageState::WaitingForStx: {
			while (ix < escaped_data.size()) {
				auto b = static_cast<uint8_t>(escaped_data[ix++]);
				if(b == STX) {
					logSerialPortSocketD() << "STX received";
					m_readMessageCrc = {};
					setReadMessageState(ReadMessageState::WaitingForEtx);
					break;
				}

				logSerialPortSocketM() << "Ignoring rubbish:" << shv::chainpack::utils::byteToHex(b) << b << static_cast<int>(b);
			}
			break;
		}
		case ReadMessageState::WaitingForEtx: {
			while (ix < escaped_data.size()) {
				auto b = static_cast<uint8_t>(escaped_data[ix++]);
				if(b == STX) {
					logSerialPortSocketD() << "STX in middle of message data received, restarting read loop";
					setReadMessageState(ReadMessageState::WaitingForStx);
					--ix;
					break;
				}
				if(b == ETX && !m_readMessageBuffer.inEscape) {
					logSerialPortSocketD() << "ETX received";
					logSerialPortSocketD() << "Message data received:" << m_readMessageBuffer.data.toHex().toStdString();
					setReadMessageState(ReadMessageState::WaitingForCrc);
					break;
				}
				m_readMessageCrc.add(b);
				if(auto err = m_readMessageBuffer.append(b); err != ReadMessageError::Ok) {
					setReadMessageError(err);
				}
			}
			break;
		}
		case ReadMessageState::WaitingForCrc: {
			while (ix < escaped_data.size()) {
				auto b = static_cast<uint8_t>(escaped_data[ix++]);
				if(b == STX) {
					logSerialPortSocketD() << "STX in middle of message data received, restarting read loop";
					setReadMessageState(ReadMessageState::WaitingForStx);
					--ix;
					break;
				}
				if(auto err = m_readMessageCrcBuffer.append(b); err != ReadMessageError::Ok) {
					setReadMessageError(err);
					break;
				}
				if(!m_readMessageCrcBuffer.inEscape && m_readMessageCrcBuffer.data.size() == sizeof(shv::chainpack::crc32_t)) {
					shv::chainpack::crc32_t msg_crc = 0;
					for(uint8_t bb : std::as_const(m_readMessageCrcBuffer.data)) {
						msg_crc <<= 8;
						msg_crc += bb;
					}
					logSerialPortSocketD() << "crc data:" << m_readMessageCrcBuffer.data.toHex().toStdString();
					logSerialPortSocketD() << "crc received:" << shv::chainpack::utils::intToHex(msg_crc);
					logSerialPortSocketD() << "crc computed:" << shv::chainpack::utils::intToHex(m_readMessageCrc.remainder());
					if(m_readMessageCrc.remainder() == msg_crc) {
						logSerialPortSocketD() << "crc OK";
						setReadMessageError(ReadMessageError::Ok);
						auto data = m_readMessageBuffer.data;
						if(data.isEmpty()) {
							// RESET message received
							logSerialPortSocketM() << "RESET message received";
							emit socketReset();
						}
						else {
							m_receivedMessages.enqueue(m_readMessageBuffer.data);
							emit readyRead();
						}
					}
					else {
						logSerialPortSocketD() << "crc ERROR";
						setReadMessageError(ReadMessageError::ErrorCrc);
					}
					break;
				}
			}
			break;
		}
		}
	}
}

void SerialPortSocket::setReadMessageState(ReadMessageState st)
{
	switch (st) {
	case ReadMessageState::WaitingForStx:
		logSerialPortSocketD() << "Entering state:" << "WaitingForStx";
		break;
	case ReadMessageState::WaitingForEtx:
		logSerialPortSocketD() << "Entering state:" << "WaitingForEtx";
		m_readMessageBuffer.clear();
		restartReceiveTimeoutTimer();
		break;
	case ReadMessageState::WaitingForCrc:
		logSerialPortSocketD() << "Entering state:" << "WaitingForCrc";
		m_readMessageCrcBuffer.clear();
		break;
	}
	m_readMessageState = st;
}

void SerialPortSocket::setReadMessageError(ReadMessageError err)
{
	m_readMessageError = err;
	if(err != ReadMessageError::Ok)
		logSerialPortSocketM() << "Set read message error:" << readMessageErrorString();
	setReadMessageState(ReadMessageState::WaitingForStx);
}

qint64 SerialPortSocket::writeBytesEscaped(const char *data, qint64 max_size)
{
	std::array arr = {char{0}};
	auto set_byte = [&arr](uint8_t b) {
		arr[0] = static_cast<char>(b);
	};
	qint64 written_cnt = 0;
	for(written_cnt = 0; written_cnt < max_size; ++written_cnt) {
		auto b = static_cast<uint8_t>(data[written_cnt]);
		if(m_escWritten) {
			//finish escape sequence
			switch (b) {
			case STX:
				set_byte(ESTX);
				break;
			case ETX:
				set_byte(EETX);
				break;
			case ESC:
				set_byte(ESC);
				break;
			default:
				logSerialPortSocketW() << "ESC followed by:" << shv::chainpack::utils::byteToHex(b) << "internal escaping error, data sent will be probably corrupted.";
				set_byte(b);
				break;
			}
		}
		else {
			switch (b) {
			case STX:
			case ETX:
			case ESC:
				set_byte(ESC);
				--written_cnt;
				break;
			default:
				set_byte(b);
				break;
			}
		}
		m_writeMessageCrc.add(arr[0]);
		auto n = m_port->write(arr.data(), 1);
		if(n < 0) {
			return -1;
		}
		if(n == 0) {
			break;
		}
		if(!m_escWritten && static_cast<uint8_t>(arr[0]) == ESC) {
			m_escWritten = true;
		}
		else {
			m_escWritten = false;
		}
	}
	return written_cnt;
}

QByteArray SerialPortSocket::readAll()
{
	if(m_receivedMessages.isEmpty())
		return {};
	return m_receivedMessages.dequeue();
}

qint64 SerialPortSocket::write(const char *data, qint64 max_size)
{
	return writeBytesEscaped(data, max_size);
}

void SerialPortSocket::writeMessageBegin()
{
	logSerialPortSocketD() << "STX sent";
	m_writeMessageCrc = {};
	std::array stx = {static_cast<char>(STX)};
	m_port->write(stx.data(), 1);
}

void SerialPortSocket::writeMessageEnd()
{
	logSerialPortSocketD() << "ETX sent";
	std::array etx = {static_cast<char>(ETX)};
	m_port->write(etx.data(), 1);
	auto crc = m_writeMessageCrc.remainder();
	static constexpr size_t N = sizeof(shv::chainpack::crc32_t);
	QByteArray crc_ba(N, 0);
	for(size_t i = 0; i < N; i++) {
#if QT_VERSION_MAJOR < 6
		crc_ba[static_cast<uint>(N - i - 1)] = static_cast<char>(crc % 256);
#else
		crc_ba[N - i - 1] = static_cast<char>(crc % 256);
#endif
		crc /= 256;
	}
	writeBytesEscaped(crc_ba.constData(), crc_ba.size());
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

SerialPortSocket::ReadMessageError SerialPortSocket::UnescapeBuffer::append(uint8_t b)
{
	if(b == STX) {
		return ReadMessageError::ErrorUnexpectedStx;
	}
	if(b == ETX) {
		return ReadMessageError::ErrorUnexpectedEtx;
	}
	if(inEscape) {
		if(b == ESTX) {
			data.append(static_cast<char>(STX));
		}
		else if(b == EETX) {
			data.append(static_cast<char>(ETX));
		}
		else if(b == ESC) {
			data.append(static_cast<char>(ESC));
		}
		else {
			return ReadMessageError::ErrorEscape;
		}
		inEscape = false;
		return ReadMessageError::Ok;
	}
	if(b == ESC) {
		inEscape = true;
		return ReadMessageError::Ok;
	}
	data.append(static_cast<char>(b));
	return ReadMessageError::Ok;
}

void SerialPortSocket::UnescapeBuffer::clear()
{
	data.clear();
	inEscape = false;
}

}
