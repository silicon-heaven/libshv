#pragma once

#include <shv/coreqt/shvcoreqtglobal.h>

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

NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QString &s);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QDateTime &d);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QDate &d);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QTime &d);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QUrl &url);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QStringList &sl);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QByteArray &ba);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QPoint &p);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QPointF &p);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QSize &sz);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QSizeF &sz);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QRect &r);
NecroLog SHVCOREQT_DECL_EXPORT operator<<(NecroLog log, const QRectF &r);

Q_DECLARE_METATYPE(NecroLog::Level)
