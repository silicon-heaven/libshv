#pragma once

#include <shv/coreqt/log.h>

#if QT_VERSION_MAJOR < 6
#include <QStringRef>

inline NecroLog operator<<(NecroLog log, const QStringRef &s)
{
	return log << s.toString().toStdString();
}
#endif

inline NecroLog operator<<(NecroLog log, const QStringView &s)
{
	return log << s.toString().toStdString();
}
