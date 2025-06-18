#include <shv/visu/timeline/sample.h>

namespace shv::visu::timeline {

Sample::Sample() = default;

Sample::Sample(timemsec_t t, const QVariant &v, bool is_repeated) : time(t), value(v), isRepeated(is_repeated)
{
}

Sample::Sample(timemsec_t t, QVariant &&v, bool is_repeated) : time(t), value(std::move(v)), isRepeated(is_repeated)
{
}

bool Sample::isValid() const
{
	return value.isValid();
}
}
