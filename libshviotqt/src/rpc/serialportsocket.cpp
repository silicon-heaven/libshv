#include "serialportsocket.h"
#include <QSerialPort>
#include <QUrl>

namespace shv::iotqt::rpc {
//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(parent)
	, m_port(port)
{
	m_port->setParent(this);

	//connect(m_port, &QSerialPort::connected, this, &Socket::connected);
	//connect(m_port, &QSerialPort::disconnected, this, &Socket::disconnected);
	connect(m_port, &QSerialPort::readyRead, this, &Socket::readyRead);
	connect(m_port, &QSerialPort::bytesWritten, this, &Socket::bytesWritten);
	//connect(m_port, &QSerialPort::stateChanged, this, [this](QLocalSocket::LocalSocketState state) {
	//	emit stateChanged(LocalSocket_convertState(state));
	//});
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
}

void SerialPortSocket::connectToHost(const QUrl &url)
{
	abort();
	setState(QAbstractSocket::ConnectingState);
	m_port->setPortName(url.path());
	shvInfo() << "opening serial port:" << m_port->portName();
	if(m_port->open(QIODevice::ReadWrite)) {
		shvInfo() << "Ok";
		// clear read buffer
		m_port->readAll();
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

QByteArray SerialPortSocket::readAll()
{
	return m_port->readAll();
}

qint64 SerialPortSocket::write(const char *data, qint64 max_size)
{
	return m_port->write(data, max_size);
}

void SerialPortSocket::writeMessageEnd()
{
	m_port->flush();
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
	// do not what to do
}
}
