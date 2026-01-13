#include "mockserialport.h"

#include <shv/chainpack/utils.h>

#include <QFile>

#include <necrolog.h>

MockSerialPort::MockSerialPort(const QString &name,  QObject *parent)
	: QSerialPort{parent}
{
	Q_ASSERT(!name.isEmpty());
	setObjectName(name);
}

bool MockSerialPort::open(OpenMode mode)
{
	Q_UNUSED(mode)
	close();
	{
		const auto tests_dir = TESTS_DIR;
		if (!tests_dir) {
			throw std::runtime_error("TESTS_DIR not set");
		}
		auto fname = QString(tests_dir) + "/serialportsocket-test-write" + objectName() + ".bin";
		m_writeFile.setFileName(fname);
		if(!m_writeFile.open(QFile::WriteOnly)) {
			throw std::runtime_error("Canot open file: " + fname.toStdString() + " for writing");
		}
	}
	QIODevice::open(mode);
	return true;
}

void MockSerialPort::close()
{
	m_writeFile.close();
	m_writtenData.clear();
	QIODevice::close();
}

void MockSerialPort::clearWrittenData()
{
	m_writtenData.clear();
}

QByteArray MockSerialPort::writtenData() const
{
	return m_writtenData;
}

void MockSerialPort::setDataToReceive(const QByteArray &d)
{
	m_dataToReceive.append(d);
	if(!d.isEmpty())
		emit readyRead();
}

qint64 MockSerialPort::writeData(const char *data, qint64 len)
{
	m_writtenData.append(data, len);
	return m_writeFile.write(data, len);
}

qint64 MockSerialPort::readData(char *data, qint64 maxlen)
{
	// FIXME: get rid of the casts here when dropping Qt5 support
	auto len = std::min(static_cast<int>(m_dataToReceive.size()), static_cast<int>(maxlen));
	for(int i = 0; i < len; ++i) {
		data[i] = m_dataToReceive[i];
	}
	m_dataToReceive = m_dataToReceive.mid(len);
	return len;
}

