#include <shv/visu/timeline/sample.h>

namespace shv::visu::timeline {

Sample::Sample() = default;

Sample::Sample(timemsec_t t, const QVariant &v) : time(t), value(v)
{
}

Sample::Sample(timemsec_t t, QVariant &&v) : time(t), value(std::move(v))
{
}

bool Sample::isValid() const
{
	return value.isValid();
}
}
