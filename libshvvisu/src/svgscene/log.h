#pragma once

#include <shv/coreqt/log.h>

#include <QStringRef>

inline NecroLog operator<<(NecroLog log, const QStringRef &s)
{
	return log << s.toString().toStdString();
}

inline NecroLog operator<<(NecroLog log, const QStringView &s)
{
	return log << s.toString().toStdString();
}
