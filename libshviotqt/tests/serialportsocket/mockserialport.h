#pragma once

#include <QSerialPort>
#include <QFile>

namespace shv::chainpack { class RpcMessage; }

class MockSerialPort : public QSerialPort
{
public:
	explicit MockSerialPort(const QString &name, QObject *parent = nullptr);

	bool open(OpenMode mode) override;
	void close() override;

	void clearWrittenData();
	QByteArray writtenData() const;
	void setDataToReceive(const QByteArray &d);
protected:
	qint64 writeData(const char *data, qint64 len) override;
	qint64 readData(char *data, qint64 maxlen) override;
private:
	QFile m_writeFile;
	QByteArray m_writtenData;
	QByteArray m_dataToReceive;
};

