#include <shv/visu/timeline/sample.h>

namespace shv::visu::timeline {

Sample::Sample() = default;

Sample::Sample(timemsec_t t, const QVariant &v, bool is_repeated) : time(t), value(v), isRepeated(is_repeated)
{
}

Sample::Sample(timemsec_t t, QVariant &&v, bool is_repeated) : time(t), value(std::move(v)), isRepeated(is_repeated)
{
}

bool Sample::operator==(const Sample &other) const
{
	return
			time == other.time  &&
			value == other.value &&
			isRepeated == other.isRepeated;

}

bool Sample::isValid() const
{
	return value.isValid();
}

Sample ChannelSamples::sampleValue(qsizetype ix) const
{
	if (ix >= count()) {
		return {};
	}
	return at(ix);
}

std::optional<qsizetype> ChannelSamples::greaterOrEqualTimeIndex(timemsec_t time) const
{
	auto it = std::upper_bound(cbegin(), cend(), time, [](const timemsec_t value, const Sample &s) {
		return s.time > value;
	});

	if (it == cend()) {
		if (last().time == time) {
			return size() - 1;
		}
		return {};
	}

	if ((it != cbegin())) {
		if (auto prev = std::prev(it); (prev->time == time) ) {
			--it;
		}
	}

	return std::distance(cbegin(), it);
}

std::optional<qsizetype> ChannelSamples::greaterTimeIndex(timemsec_t time) const
{
	auto it = std::upper_bound(cbegin(), cend(), time, [](const timemsec_t value, const Sample &s) {
		return s.time > value;
	});

	if (it == cend()) {
		return {};
	}

	return std::distance(cbegin(), it);
}

std::optional<qsizetype> ChannelSamples::lessOrEqualTimeIndex(timemsec_t time) const
{
	auto it = std::lower_bound(cbegin(), cend(), time, [](const Sample &s, const timemsec_t value) {
		return s.time < value;
	});

	if (it == cend()) {
		if (!isEmpty()) {
			return size() - 1;
		}
	}
	else {
		if (it->time != time) {
			if (it == cbegin()) {
				return {};
			}

			it--;
		}
	}

	return std::distance(cbegin(), it);
}

std::optional<qsizetype> ChannelSamples::lessTimeIndex(timemsec_t time) const
{
	auto ix = lessOrEqualTimeIndex(time);
	if (!ix) {
		return {};
	}

	Sample s = sampleValue(ix.value());
	if (s.time == time) {
		if (auto ret = ix.value() - 1;  ret >= 0) {
			return ret;
		}

		return {};
	}
	return ix;
}

}

