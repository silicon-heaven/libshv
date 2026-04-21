#pragma once

#include <shv/visu/shvvisu_export.h>

#include <QString>
#include <QDateTime>
#include <QColor>

namespace shv::visu::toast {

class LIBSHVVISU_EXPORT ToastMessage
{
public:
	enum class MessageType {Info, Success, Warning, Error};
	static QColor toastMessageTypeToColor(MessageType type);
	static void setMessageTypeColor(MessageType type, const QColor &color, bool dark_theme);
	static QString toastMessageTypeToString(MessageType type);

	ToastMessage() = default;
	ToastMessage(const QString &message, MessageType message_type, int duration_secs);

	QString message() const;
	MessageType messageType() const;
	QDateTime creationTimestamp() const;
	int durationSecs() const;

private:
	QString m_message;
	MessageType m_type = MessageType::Info;
	QDateTime m_creationTimestamp;
	int m_duration_secs;
};

}
