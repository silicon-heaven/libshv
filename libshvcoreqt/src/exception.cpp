#include <shv/coreqt/exception.h>

namespace shv::coreqt {

Exception::Exception(const QString &msg, const QString &where)
	: shv::core::Exception(msg.toStdString(), where.toStdString())
{
}

}
