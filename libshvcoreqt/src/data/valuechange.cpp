#include "valuechange.h"

#include "../exception.h"

namespace shv {
namespace coreqt {
namespace data {

SerieData::const_iterator SerieData::lessOrEqualIterator(ValueChange::ValueX value_x) const
{
	const_iterator it = lower_bound(value_x);

	if (it == cend()) {
		if (!empty()) {
			it--;
		}
	}
	else {
		if (!compareValueX(it->valueX, value_x, m_xType)) {
			if (it == cbegin()) {
				it = cend();
			}
			else {
				it--;
			}
		}
	}
	return it;
}

QPair<SerieData::const_iterator, SerieData::const_iterator> SerieData::intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const
{
	QPair<const_iterator, const_iterator> result;
	result.first = lessOrEqualIterator(start);
	result.second = lessOrEqualIterator(end);

	if (result.first == cend() && !empty()){
		result.first = cbegin();
	}

	valid = (result.second != cend());
	return result;
}

ValueXInterval SerieData::range() const
{
	if (size()) {
		return ValueXInterval(at(0).valueX, back().valueX, xType());
	}
	else {
		switch (xType()) {
		case ValueType::Double:
			return ValueXInterval(0.0, 0.0);
		case ValueType::Int:
			return ValueXInterval(0, 0);
		case ValueType::TimeStamp:
			return ValueXInterval(0LL, 0LL);
		default:
			SHV_EXCEPTION("Invalid type on X axis");
		}
	}
}

bool SerieData::addValueChange(const ValueChange &value, bool check_changes)
{
	int sz = size();
	if (sz == 0) {
		push_back(value);
		return true;
	}
	else {
		const ValueChange &last = at(sz - 1);
		if (!check_changes || (!compareValueX(last, value, xType()) && !compareValueY(last, value, yType()))) {
			push_back(value);
			return true;
		}
	}
	return false;
}

SerieData::iterator SerieData::insertValueChange(const_iterator position, const ValueChange &value)
{
	return insert(position, value);
}

void SerieData::updateValueChange(const_iterator position, const ValueChange &new_value)
{
	Q_ASSERT(position >= cbegin());
	unsigned long index = (unsigned long)(position - cbegin());
	ValueChange &old_value = at(index);
	if (!compareValueX(old_value, new_value, m_xType)) {
		if ((index > 0 && (compareValueX(new_value, at(index - 1), m_xType) || lessThenValueX(new_value, at(index - 1), m_xType)))
			||
			(index < size() - 1 && (compareValueX(new_value, at(index + 1), m_xType) || greaterThenValueX(new_value, at(index + 1), m_xType)))) {
			SHV_EXCEPTION("updateValueChange: requested change of ValueX would break time sequence");
		}
	}
	old_value = new_value;
}

void SerieData::extendRange(int &min, int &max) const
{
	if (size()) {
		if (at(0).valueX.intValue < min) {
			min = at(0).valueX.intValue;
		}
		if (back().valueX.intValue > max) {
			max = back().valueX.intValue;
		}
	}
}

void SerieData::extendRange(double &min, double &max) const
{
	if (size()) {
		if (at(0).valueX.doubleValue < min) {
			min = at(0).valueX.doubleValue;
		}
		if (back().valueX.doubleValue > max) {
			max = back().valueX.doubleValue;
		}
	}
}

void SerieData::extendRange(ValueChange::TimeStamp &min, ValueChange::TimeStamp &max) const
{
	if (size()) {
		if (at(0).valueX.timeStamp < min) {
			min = at(0).valueX.timeStamp;
		}
		if (back().valueX.timeStamp > max) {
			max = back().valueX.timeStamp;
		}
	}
}

bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return compareValueX(value1.valueX, value2.valueX, type);
}

bool compareValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp == value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return compareValueY(value1.valueY, value2.valueY, type);
}

bool compareValueY(const ValueChange::ValueY &value1, const ValueChange::ValueY &value2, ValueType type)
{
	switch (type) {
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	case ValueType::Bool:
		return value1.boolValue == value2.boolValue;
	case ValueType::CustomDataPointer:
		return value1.pointerValue == value2.pointerValue;
	default:
		SHV_EXCEPTION("Invalid type on valueY");
	}
}

SerieData::const_iterator SerieData::upper_bound(ValueChange::ValueX value_x) const
{
	return upper_bound(cbegin(), cend(), value_x);
}

SerieData::const_iterator SerieData::upper_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const
{
	auto it = cend();
	if (m_xType == ValueType::TimeStamp) {
		it = std::upper_bound(begin, end, value_x.timeStamp, [](ValueChange::TimeStamp x, ValueChange val) {
			return val.valueX.timeStamp > x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::upper_bound(begin, end, value_x.intValue, [](int x, ValueChange val) {
			return val.valueX.intValue > x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::upper_bound(begin, end, value_x.doubleValue, [](double x, ValueChange val) {
			return val.valueX.doubleValue > x;
		});
	}
	return it;
}

SerieData::const_iterator SerieData::lower_bound(ValueChange::ValueX value_x) const
{
	return lower_bound(cbegin(), cend(), value_x);
}

SerieData::const_iterator SerieData::lower_bound(SerieData::const_iterator begin, SerieData::const_iterator end, ValueChange::ValueX value_x) const
{
	auto it = cend();
	if (m_xType == ValueType::TimeStamp) {
		it = std::lower_bound(begin, end, value_x.timeStamp, [](ValueChange val, ValueChange::TimeStamp x) {
			return val.valueX.timeStamp < x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::lower_bound(begin, end, value_x.intValue, [](ValueChange val, int x) {
			return val.valueX.intValue < x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::lower_bound(begin, end, value_x.doubleValue, [](ValueChange val, double x) {
			return val.valueX.doubleValue < x;
		});
	}
	return it;
}

SerieData::const_iterator SerieData::findMinYValue(const_iterator begin, const_iterator end, const ValueChange::ValueX x_value) const
{
	auto it = lower_bound(begin, end, x_value);
	if (it != begin) {
		--it;
	}
	return it;
}

SerieData::const_iterator SerieData::findMinYValue(const ValueChange::ValueX x_value) const
{
	return findMinYValue(cbegin(), cend(), x_value);
}

bool ValueXInterval::operator==(const ValueXInterval &interval) const
{
	if (type != interval.type) {
		return false;
	}
	return compareValueX(min, interval.min, type) && compareValueX(max, interval.max, type);
}

ValueChange::ValueX ValueXInterval::length() const
{
	if (type == ValueType::TimeStamp) {
		return ValueChange::ValueX(max.timeStamp - min.timeStamp);
	}
	else if (type == ValueType::Int) {
		return ValueChange::ValueX(max.intValue - min.intValue);
	}
	else if (type == ValueType::Double) {
		return ValueChange::ValueX(max.doubleValue - min.doubleValue);
	}
    SHV_EXCEPTION("Invalid interval type");
}

bool lessThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return lessThenValueX(value1.valueX, value2.valueX, type);
}

bool lessThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp < value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue < value2.doubleValue;
	case ValueType::Int:
		return value1.intValue < value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

bool greaterThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return greaterThenValueX(value1.valueX, value2.valueX, type);
}

bool greaterThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp > value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue > value2.doubleValue;
	case ValueType::Int:
		return value1.intValue > value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

}
}
}
