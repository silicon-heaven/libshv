#pragma once

#include <shv/visu/shvvisuglobal.h>

#include <QVariant>

#include <limits>

namespace shv::visu::timeline {

using timemsec_t = int64_t;

struct SHVVISU_DECL_EXPORT Sample
{

	timemsec_t time = 0;
	QVariant value;

	Sample();
	Sample(timemsec_t t, const QVariant &v);
	Sample(timemsec_t t, QVariant &&v);

	bool isValid() const;
};

template<typename T>
struct Range
{
	T min;
	T max;

	Range() : min(1), max(0) {} // invalid range
	Range(T mn, T mx) : min(mn), max(mx) {}

	auto operator<=>(const Range&) const = default;

	Range& normalize() {if (min > max)  std::swap(min, max); return *this; }
	Range normalized() const {auto r = *this; r.normalize(); return r;}
	T interval() const { return max - min; }
	bool isEmpty() const { return interval() == 0; }
	bool isValid() const { return max >= min; }
	bool contains(T t) const { return isValid() && (t >= min) && (t <= max); }
	Range<T> united(const Range<T> &o) const {
		if (isValid()) {
			if (o.isValid()) {
				auto ret = *this;
				ret.min = std::min(ret.min, o.min);
				ret.max = std::max(ret.max, o.max);
				return ret;
			}
			return *this;
		}
		return o;
	}
	Range<T> intersected(const Range<T> &o) const {
		if (isValid()) {
			if (o.isValid()) {
				auto ret = *this;
				ret.min = std::max(ret.min, o.min);
				ret.max = std::min(ret.max, o.max);
				return ret;
			}
			return o;
		}
		return *this;
	}
};

template<> inline bool Range<double>::isEmpty() const { return interval() < 1e-42; }

using XRange = Range<timemsec_t>;
using YRange = Range<double>;

}
