#include <shv/visu/toast/toastmessage.h>

#include <QGuiApplication>
#include <QPalette>

#include <map>

namespace shv::visu::toast {

namespace {
	std::map<ToastMessage::MessageType, QColor>& lightThemeColors()
	{
		static std::map<ToastMessage::MessageType, QColor> instance {
			{ ToastMessage::MessageType::Info,    QColor("#bae1ff") },
			{ ToastMessage::MessageType::Success, QColor("#baffc9") },
			{ ToastMessage::MessageType::Warning, QColor("#ffdfba") },
			{ ToastMessage::MessageType::Error,   QColor("#ffb3ba") }
		};
		return instance;
	}

	std::map<ToastMessage::MessageType, QColor>& darkThemeColors()
	{
		static std::map<ToastMessage::MessageType, QColor> instance {
			{ ToastMessage::MessageType::Info,    QColor("#7fbfe0") },
			{ ToastMessage::MessageType::Success, QColor("#6fcf81") },
			{ ToastMessage::MessageType::Warning, QColor("#f7c36a") },
			{ ToastMessage::MessageType::Error,   QColor("#f28b8b") }
		};
		return instance;
	}
}

QColor ToastMessage::toastMessageTypeToColor(ToastMessage::MessageType type)
{
	const bool is_dark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
	const auto &colors = is_dark ? darkThemeColors() : lightThemeColors();

	if (auto it = colors.find(type); it != colors.end()) {
		return it->second;
	}

	return Qt::gray;
}

void ToastMessage::setMessageTypeColor(MessageType type, const QColor &color, bool dark_theme)
{
	auto &map = dark_theme ? darkThemeColors() : lightThemeColors();
	map[type] = color;
}


QString ToastMessage::toastMessageTypeToString(MessageType type)
{
	switch (type) {
	case ToastMessage::MessageType::Info: return "Info";
	case ToastMessage::MessageType::Success: return "Success";
	case ToastMessage::MessageType::Warning: return "Warning";
	case ToastMessage::MessageType::Error: return "Error";
	}

	return QString();
}

ToastMessage::ToastMessage(const QString &message, MessageType message_type, int duration_secs)
{
	m_message = message;
	m_type = message_type;
	m_duration_secs = duration_secs;
	m_creationTimestamp = QDateTime::currentDateTime();
}

QString ToastMessage::message() const
{
	return m_message;
}

ToastMessage::MessageType ToastMessage::messageType() const
{
	return m_type;
}

QDateTime ToastMessage::creationTimestamp() const
{
	return m_creationTimestamp;
}

int ToastMessage::durationSecs() const
{
	return m_duration_secs;
}
}
