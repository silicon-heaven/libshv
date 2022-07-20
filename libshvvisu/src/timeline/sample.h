#pragma once

#include "../shvvisuglobal.h"

#include <QVariant>

#include <limits>

namespace shv {
namespace visu {
namespace timeline {

using timemsec_t = int64_t;

struct SHVVISU_DECL_EXPORT Sample
{

	timemsec_t time = 0;
	QVariant value;
	//timemsec_t origtime = 0; // sometimes is needed to show samples in transformed time scale (hide empty areas without samples)

	Sample() {}
	Sample(timemsec_t t, const QVariant &v) : time(t), value(v) {}
	Sample(timemsec_t t, QVariant &&v) : time(t), value(std::move(v)) {}

	bool isValid() const {return value.isValid() && time > 0;}
};

template<typename T>
struct Range
{
	T min = std::numeric_limits<T>::max();
	T max = std::numeric_limits<T>::min();

	Range() {}
	Range(T mn, T mx) : min(mn), max(mx) {}
	//XRange(const QPair<timemsec_t, timemsec_t> r) : min(r.first), max(r.second) {}

	//bool operator==(const T &o) const { return min == o.min && max == o.max; }

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

}}}

