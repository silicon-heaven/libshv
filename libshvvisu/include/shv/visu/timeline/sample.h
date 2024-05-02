#pragma once

#include "../shvvisuglobal.h"

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

	Range& normalize() {if (min > max)  std::swap(min, max); return *this; }
	bool isValid() const { return interval() >= 0; }
	bool isEmpty() const { return interval() == 0; }
	bool contains(T t) const { return ((t >= min) && (t <= max)); }
	T interval() const {return max - min;}
	Range<T> united(const Range<T> &o) const {
		if(isValid() && o.isValid()) {
			Range<T> ret = *this;
			ret.min = std::min(ret.min, o.min);
			ret.max = std::max(ret.max, o.max);
			return ret;
		}
		else if(isValid()) {
			return *this;
		}
		else {
			return o;
		}
	}
};

template<> inline bool Range<double>::isEmpty() const { return interval() < 1e-42; }

using XRange = Range<timemsec_t>;
using YRange = Range<double>;

}
