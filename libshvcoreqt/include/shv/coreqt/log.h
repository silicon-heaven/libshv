#pragma once

#include <shv/coreqt/shvcoreqt_export.h>

#include <shv/core/log.h>

#include <QMetaType>

class QString;
class QByteArray;
class QMetaType;
class QString;
class QDateTime;
class QDate;
class QTime;
class QPoint;
class QPointF;
class QSize;
class QSizeF;
class QRect;
class QRectF;
class QUrl;

NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QString &s);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QDateTime &d);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QDate &d);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QTime &d);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QUrl &url);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QStringList &sl);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QByteArray &ba);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QPoint &p);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QPointF &p);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QSize &sz);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QSizeF &sz);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QRect &r);
NecroLog LIBSHVCOREQT_EXPORT operator<<(NecroLog log, const QRectF &r);

Q_DECLARE_METATYPE(NecroLog::Level)
