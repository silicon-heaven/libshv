#pragma once

#include <shv/coreqt/shvcoreqt_export.h>

#include <shv/core/exception.h>

#include <QString>

namespace shv::coreqt {

class LIBSHVCOREQT_EXPORT Exception : public shv::core::Exception
{
public:
	Exception(const QString &msg, const QString &where = QString());
};
}

#define SHV_QT_EXCEPTION(e) throw shv::coreqt::Exception(e, QString(__FILE__) + ":" + QString::number(__LINE__))
