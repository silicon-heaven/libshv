#include <shv/core/log.h>

NecroLog operator<<(NecroLog log, const shv::core::StringView &v)
{
	return log.operator<<(v);
}
